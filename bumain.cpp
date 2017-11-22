#include "bumain.h"
#include "ui_bumain.h"
#include <QDebug>
#include <qdebug.h>
#include <QThread>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QApplication>
#include <QDesktopWidget>
#include <QScrollBar>
#include <QMimeData>
#include <QtCore>
#include <QFile>

BUMain::BUMain(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::BUMain)
{

    ui->setupUi(this);

    dialog = new QFileDialog(this);
    gobLogViewer = new LogViewer;
    gobSearchDialog = new SearchDialog(this);
    targetDirectories = new QStringList;
    sourceFiles = new QStringList;

    giCurrentPos = -1;
    giCurrentNumPos = -1;
    giProgress = 0;

    giKeep = 0;
    giCopyFileIndex = 0;
    validatorFlag = 0;
    giErrorOnCopyFlag = 0;

    gbBackcupButtonPressed = false;
    gbCountCancel = false;

    QFile styleFile("style.qss");
    if(styleFile.exists()){
        if(styleFile.open(QFile::ReadOnly)){
            QString StyleSheet = QLatin1String(styleFile.readAll());
            this->setStyleSheet(StyleSheet);
        }
    }
    this->resetCounters();
    this->installEventFilters();
    this->move(QApplication::desktop()->screen()->rect().center() - this->rect().center());
    this->ui->backupButton->setEnabled(false);
    this->ui->openTargetButton->setEnabled(false);
    this->initThreadSetup();
    this->loadSessionFile("Session.txt");
}

bool BUMain::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == (QObject*)ui->originButton) {
        if (event->type() == QEvent::Enter){
            ui->statusBar->showMessage("Select the files to copy",TOOLTIP_DURATION);
        }
    }else if(obj == (QObject*)ui->logViewerButton){
        if (event->type() == QEvent::Enter){
            ui->statusBar->showMessage("Open the log viewer",TOOLTIP_DURATION);
        }
    }else if(obj == (QObject*)ui->targetButton){
        if (event->type() == QEvent::Enter){
            ui->statusBar->showMessage("Select target folder",TOOLTIP_DURATION);
        }
    }else if(obj == (QObject*)ui->backupButton){
        if (event->type() == QEvent::Enter){
            ui->statusBar->showMessage("Start copy",TOOLTIP_DURATION);
        }
    }else if(obj == (QObject*)ui->openTargetButton){
        if (event->type() == QEvent::Enter){
            ui->statusBar->showMessage("Open the target folder",TOOLTIP_DURATION);
        }
    }else if(obj == (QObject*)ui->backupButton && gbBackcupButtonPressed == true){
        if (event->type() == QEvent::Enter){
            ui->statusBar->showMessage("Cancel copy",TOOLTIP_DURATION);
        }
    }
    return QWidget::eventFilter(obj, event);
}

BUMain::~BUMain()
{
    delete ui;
    thread->quit();
    thread->wait();
}

/**
  Threads and connections setup.
*/
void BUMain::initThreadSetup()
{
    thread = new QThread;
    worker = new Worker;
    worker->moveToThread(thread);
    connect(worker,SIGNAL(worker_Signal_updateProgressBar(int)),ui->overallCopyProgressBar,SLOT(setValue(int)));
    connect(this,SIGNAL(main_signal_createDirs(QString,QString,int)),worker,SLOT(worker_slot_createDirs(QString,QString,int)));
    connect(worker,SIGNAL(worker_signal_keepCopying()),this,SLOT(main_slot_keepCopying()));
    connect(worker,SIGNAL(worker_signal_logInfo(QString)),gobLogViewer,SLOT(logger_slot_logInfo(QString)));
    connect(worker,SIGNAL(worker_signal_statusInfo(QString)),this,SLOT(main_slot_setStatus(QString)));
    connect(worker,SIGNAL(worker_signal_showMessage(QString,int)),this,SLOT(main_slot_showMessage(QString,int)));
    connect(this,SIGNAL(main_signal_setStopFlag(int)),worker,SLOT(worker_slot_setStopFlag(int)));
    connect(this,SIGNAL(main_signal_readyToStartCopy()),worker,SLOT(worker_slot_readyToStartCopy()));
    connect(worker,SIGNAL(worker_signal_sendDirAndFileList(QStringList*,QStringList*)),this,SLOT(main_slot_receiveDirAndFileList(QStringList*,QStringList*)));
    connect(this,SIGNAL(main_signal_copyFile(QString,QString)),worker,SLOT(worker_slot_copyFile(QString,QString)));
    connect(worker,SIGNAL(worker_signal_copyNextFile()),this,SLOT(main_slot_copyNextFile()));
    connect(this,SIGNAL(main_signal_logInfo(QString)),gobLogViewer,SLOT(logger_slot_logInfo(QString)));
    connect(gobSearchDialog,SIGNAL(search_signal_resetCursor()),this,SLOT(main_slot_resetCursor()));
    connect(this,SIGNAL(main_signal_scanFolders(QString)),worker,SLOT(worker_slot_scanFolders(QString)));
    connect(worker,SIGNAL(worker_signal_setTotalFilesAndFolders(int,int,qint64)),this,SLOT(main_slot_setTotalFilesAndFolders(int,int,qint64)));
    connect(gobSearchDialog,SIGNAL(search_signal_getTextEditText()),this,SLOT(main_slot_getTextEdit()));
    connect(this,SIGNAL(main_signal_setTextEdit(QPlainTextEdit*)),gobSearchDialog,SLOT(search_slot_setTextEdit(QPlainTextEdit*)));
    connect(worker,SIGNAL(worker_signal_scanReady()),this,SLOT(main_slot_scanReady()));
    connect(this,SIGNAL(main_signal_scanNextPath()),worker,SLOT(worker_slot_scanNextPath()));
    connect(gobSearchDialog, SIGNAL(search_signal_enableFilescan()), this, SLOT(main_slot_enableFileScan()));
    connect(gobSearchDialog, SIGNAL(search_signal_disableFilescan()), this, SLOT(main_slot_disableFileScan()));
    connect(this->ui->fromFilesTextArea,SIGNAL(processDropEvent(QDropEvent*)),this,SLOT(main_slot_processDropEvent(QDropEvent*)));
    connect(this,SIGNAL(main_signal_renameEnable(bool)),worker,SLOT(worker_slot_renameEnable(bool)));
    connect(worker,SIGNAL(worker_signal_errorOnCopy()),this,SLOT(main_slot_errorOnCopy()));
    connect(worker,SIGNAL(worker_signal_workerDone()), this,SLOT(main_slot_workerDone()));
    connect(thread,SIGNAL(finished()),worker,SLOT(deleteLater()));
    connect(thread,SIGNAL(finished()),thread,SLOT(deleteLater()));

    thread->start();
}

/**
  Event filters installer. Allow showing the button help in the
  status bar.
*/
void BUMain::installEventFilters()
{
    ui->originButton->installEventFilter(this);
    ui->targetButton->installEventFilter(this);
    ui->logViewerButton->installEventFilter(this);
    ui->openTargetButton->installEventFilter(this);
    ui->backupButton->installEventFilter(this);
    ui->fromFilesTextArea->installEventFilter(this);
}

/**
  Event filter uninstaller. Disable the button help, in the status bar.
*/
void BUMain::uninstallEventFilters()
{
    ui->originButton->removeEventFilter(this);
    ui->targetButton->removeEventFilter(this);
    ui->logViewerButton->removeEventFilter(this);
    ui->openTargetButton->removeEventFilter(this);
    ui->backupButton->removeEventFilter(this);
    ui->fromFilesTextArea->removeEventFilter(this);
}

/**
  Save the current session to a text file.
  @param filePath File path to be saved
  @return true if the save was successful, false otherwise.
*/
bool BUMain::saveSessionToFile(QString filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
           return false;

    QTextStream out(&file);
    out << ui->fromFilesTextArea->toPlainText() + "\n--" + ui->toFilesTextField->text();
    file.close();
    return true;
}

/**
  Action triggered when the backup or cancel button is clicked.
*/
void BUMain::on_backupButton_clicked()
{
    giErrorOnCopyFlag = 0;
    QString lsCurrentPath = "";
    if(gbBackcupButtonPressed == false){

        int recursiveAlertFlag = 0;

        worker->setFileCounter(0);

        gobPaths = ui->fromFilesTextArea->toPlainText().split(",");
        gobPaths.removeAt(gobPaths.length() - 1);
        //remove commented paths
        for(int i = 0; i < gobPaths.length(); i++){
            lsCurrentPath = gobPaths.at(i);
            if(lsCurrentPath.startsWith("#") || lsCurrentPath.startsWith("\n#")){
                gobPaths.removeAt(i);
                i --;
            }
        }

        for(int i = 0; i < gobPaths.length(); i++){
            gobPaths.replace(i,gobPaths.at(i).trimmed());
            lsCurrentPath = gobPaths.at(i);
            lsCurrentPath = removeTrailingSlashes(lsCurrentPath);
            if(ui->toFilesTextField->text().contains(lsCurrentPath)){
                recursiveAlertFlag = 1;
                i = gobPaths.length() + 2;
            }
        }

        QString firstPath = gobPaths.at(0);

        if(recursiveAlertFlag == 0)
        {
            giKeep = 0;
            giProgress = 0;
            giCopyFileIndex = 0;
            gbBackcupButtonPressed = true;

            this->uninstallEventFilters();
            this->ui->toFilesTextField->setText(ui->toFilesTextField->text().trimmed());
            this->ui->backupButton->setText("Ca&ncel");
            this->ui->backupButton->setIcon(QIcon(":/icons/cancelCopy.png"));

            emit(main_signal_logInfo("\n ----- Copy Started ----- \n"));
            emit(main_signal_setStopFlag(0));
            if(firstPath.contains(">") && firstPath.lastIndexOf(">") != firstPath.length()){
                //qDebug() << "Custom path found";
                emit main_signal_createDirs(firstPath.split(">").at(0),firstPath.split(">").at(1),giKeep);
            }else{
                emit main_signal_createDirs(gobPaths.at(0),ui->toFilesTextField->text().trimmed(),giKeep);
            }
        }else{
            this->main_slot_showMessage("Recursive operation! \nThe target folder contains a source folder",QMessageBox::Critical);
        }
    }else{

        gbBackcupButtonPressed = false;

        this->ui->backupButton->setText("&Backup!");
        this->ui->backupButton->setIcon(QIcon(":/icons/backupButton.png"));

        emit(main_signal_logInfo("\n ----- Copy Cancelled ----- \n"));
        on_cancelButton_clicked();
    }
}

/**
  Action triggered when the origin button is clicked.
*/
void BUMain::on_originButton_clicked()
{
    targetFolder.clear();
    giTmpTotalFolders = 0;

    dialog->setFileMode(QFileDialog::Directory);
    dialog->setOption(QFileDialog::ShowDirsOnly,true);
    dialog->setOption(QFileDialog::HideNameFilterDetails);
    if(dialog->exec()){
        targetFolder = dialog->selectedFiles().at(0);
    }

    if(targetFolder != NULL && targetFolder != ""){
        QDir dir(targetFolder);
        dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Hidden | QDir::System);
        dir.setSorting(QDir::Type);
        if(dir.entryList().length() != 0) ui->fromFilesTextArea->clear();
        ui->fromFilesTextArea->appendPlainText(dir.absolutePath() + ",");
        if(validatorFlag == 1) ui->backupButton->setEnabled(true);
    }
}

/**
  Action triggered when the target button is clicked.
*/
void BUMain::on_targetButton_clicked()
{
    dialog->setFileMode(QFileDialog::Directory);
    dialog->setOption(QFileDialog::ShowDirsOnly,true);
    if(dialog->exec()){
        targetFolder = dialog->selectedFiles().at(0);
    }

    if(targetFolder != NULL && targetFolder != "" && giTmpTotalFiles > 0){
        ui->backupButton->setEnabled(true);
    }

    ui->toFilesTextField->setText(targetFolder);
}

/**
  Action triggered when the text in the target line, is modified.
*/
void BUMain::on_toFilesTextField_textChanged()
{
    validatorFlag = 0;
    if(!ui->toFilesTextField->text().isEmpty() && !ui->toFilesTextField->text().startsWith(" "))
    {
        ui->openTargetButton->setEnabled(true);
        validatorFlag = 1;

        if(giTotalFiles > 0){
            ui->backupButton->setEnabled(true);
        }

    }else{
        ui->backupButton->setEnabled(false);
        ui->openTargetButton->setEnabled(false);
    }
}

/**
  Slot triggered when the worker request for a new copy of a single file. This slot
  is called only one time.
*/
void BUMain::main_slot_keepCopying()
{
    giProgress ++;

    if(giProgress < gobPaths.length() && giKeep == 0){
        QString currentPath = gobPaths.at(giProgress);
        if(currentPath.contains(">") && currentPath.lastIndexOf(">") != currentPath.length() - 1){
            emit(main_signal_createDirs(currentPath.split(">").at(0),currentPath.split(">").at(1),giKeep));
        }else {
            if(currentPath.contains(">")){
                currentPath = currentPath.split(">").at(0);
            }
            emit(main_signal_createDirs(currentPath,ui->toFilesTextField->text().trimmed(),giKeep));
        }
    }else{
        this->installEventFilters();
        //qDebug() << "main: Scan finished, emmiting SIGNAL readyToStartCopy";
        emit(main_signal_readyToStartCopy());
        this->main_slot_setStatus("Ready.");
    }
}

/**
  Slot triggered when any class need to show a message in the status bar.
  @param Message to be shown.
*/
void BUMain::main_slot_setStatus(QString status)
{
    ui->statusBar->showMessage(status);
}

/**
  Slot triggered whe the worker sends the files to be copied and their destination folders.
*/
void BUMain::main_slot_receiveDirAndFileList(QStringList *dirs, QStringList *files)
{
    sourceFiles = files;
    targetDirectories = dirs;
    //qDebug() << "main: Emmiting signal copyFile with " << sourceFiles->at(giCopyFileIndex) << ", " << targetDirectories->at(giCopyFileIndex);
    emit(main_signal_copyFile(sourceFiles->at(giCopyFileIndex),targetDirectories->at(giCopyFileIndex)));

}

/**
  Slot triggered when the worker request for a new single file copy. This slot
  is called for every file to be copied.
*/
void BUMain::main_slot_copyNextFile()
{
    //qDebug() << "main: copyNextFile SIGNAL received ";
    giCopyFileIndex ++;
    if(giKeep == 0 && giCopyFileIndex < sourceFiles->length()){
        //qDebug() << "main: Emmiting SIGNAL copyFile with " << sourceFiles->at(giCopyFileIndex) << ", " << targetDirectories->at(giCopyFileIndex);
        emit(main_signal_copyFile(sourceFiles->at(giCopyFileIndex),targetDirectories->at(giCopyFileIndex)));
    }else{
        //qDebug() << "Copy finished.";
        if(giErrorOnCopyFlag == 0) this->main_slot_showMessage("Copy Finished",QMessageBox::Information);
        else this->main_slot_showMessage("Copy Finished with errors! \nSee the log for details.",QMessageBox::Critical);
    }
}

/**
  Slot triggered when the text cursor of the sources text area needs to be reseted.
*/
void BUMain::main_slot_resetCursor()
{
    //qDebug() << "main: resetCursor SIGNAL Recevied";
    ui->fromFilesTextArea->moveCursor(QTextCursor::Start);
}

void BUMain::main_slot_setTotalFilesAndFolders(int aiFileCounter,int aiFolderCounter, qint64 aiTotalFilesSize)
{
    giTmpTotalFolders = aiFolderCounter;
    giTmpTotalFiles = aiFileCounter;
    giTmpTotalFilesSize = aiTotalFilesSize;

    ui->fileCountLabel->setText(aiFileCounter > 0 ? QString::number(aiFileCounter):QString::number(0));
    ui->folderCountLabel->setText(aiFolderCounter > 0 ? QString::number(aiFolderCounter):QString::number(0));
    ui->totalFileSizeValueLabel->setText(aiTotalFilesSize > 0 ? QString::number(aiTotalFilesSize):QString::number(0));
}

/**
  Slot triggered wheb the counting of files and folders ends.
*/
void BUMain::main_slot_workerDone()
{

    //qDebug() << "main: SIGNAL workerDone received";
    giPathIndex ++;
    giTotalFiles += giTmpTotalFiles;
    giTotalFilesSize += giTmpTotalFilesSize;
    giTotalFolders += giTmpTotalFolders;
    if(giPathIndex < gobPaths.length()){
        emit(main_signal_scanFolders(gobPaths.at(giPathIndex)));

    }else{
        gobPaths.clear();
        this->installEventFilters();
        ui->fileCountLabel->setText(giTotalFiles > 0 ? QString::number(giTotalFiles):QString::number(0));
        ui->folderCountLabel->setText(giTotalFolders > 0 ? QString::number(giTotalFolders):QString::number(0));
        ui->totalFileSizeValueLabel->setText(giTotalFilesSize > 0 ? QString::number(giTotalFilesSize):QString::number(0));
        ui->overallCopyProgressBar->setMaximum(giTotalFiles > 0 ? giTotalFiles : 1);
        checkBackupButton();
    }
}

void BUMain::main_slot_getTextEdit()
{
    emit(main_signal_setTextEdit(this->ui->fromFilesTextArea));
}

void BUMain::on_openTargetButton_clicked()
{
    ui->toFilesTextField->setText(ui->toFilesTextField->text().trimmed());

    if(ui->toFilesTextField->text() != NULL &&
            ui->toFilesTextField->text() != "" &&
            !ui->toFilesTextField->text().startsWith(' '))
    {
        if(!QDesktopServices::openUrl(QUrl::fromLocalFile(ui->toFilesTextField->text())))
        {
            this->main_slot_showMessage("Target folder \"" + ui->toFilesTextField->text() + "\" not found!",QMessageBox::Critical);
        }
    }
}

void BUMain::on_fromFilesTextArea_textChanged()
{
    disconnect(this,SIGNAL(main_signal_scanFolders(QString)),worker,SLOT(worker_slot_scanFolders(QString)));
    gbCountCancel = true;
    resetCounters();
    QString lsCurrentPath = "";
    QString lsAreaText = ui->fromFilesTextArea->toPlainText();

    if(((lsAreaText.lastIndexOf(",") != giCurrentPos
            && lsAreaText.lastIndexOf(",") != -1)
            || lsAreaText.lastIndexOf("#") != giCurrentNumPos)){

        ui->overallCopyProgressBar->setMaximum(0);
        gobPaths.clear();
        giCurrentPos = lsAreaText.lastIndexOf(",");
        gobPaths = lsAreaText.split(",");
        gobPaths.removeAt(gobPaths.length() - 1);
        this->uninstallEventFilters();
        for(int i = 0; i < gobPaths.length(); i++){
            lsCurrentPath = gobPaths.at(i);
            if(lsCurrentPath.startsWith("#") || lsCurrentPath.startsWith("\n#")){
                gobPaths.removeAt(i);
                i--;
            }
        }
        giCurrentNumPos = lsAreaText.lastIndexOf("#");
        if(gobPaths.length() > 0) {
            connect(this,SIGNAL(main_signal_scanFolders(QString)),worker,SLOT(worker_slot_scanFolders(QString)));
            gbCountCancel = false;
            emit(main_signal_scanFolders(gobPaths.at(giPathIndex)));
        }else{
            resetState();
        }

    }else if(lsAreaText.lastIndexOf(",") == -1){

        resetState();
    }
}

void BUMain::main_slot_scanReady()
{
    //qDebug() << "main: scanReady SIGNAL received";
    if(gbCountCancel == false ){
        //qDebug() << "main: Emitting scanNextPath SIGNAL";
        emit(main_signal_scanNextPath());
    }
}

void BUMain::main_slot_disableFileScan()
{
    //qDebug() << "main: disableFileScan SIGNAL received";
    this->blockSignals(true);
    worker->blockSignals(true);
}

void BUMain::main_slot_enableFileScan()
{
    //qDebug() << "main: enableFileScan SIGNAL received";
    this->blockSignals(false);
    worker->blockSignals(false);
    this->on_fromFilesTextArea_textChanged();
}

void BUMain::main_slot_showMessage(QString message, int messageType)
{
    giCopyFileIndex = 0;
    this->ui->backupButton->setText("&Backup!");
    this->ui->backupButton->setIcon(QIcon(":/icons/backupButton.png"));
    gbBackcupButtonPressed = false;

    switch (messageType) {
    case QMessageBox::Critical:
        QMessageBox::critical(this,"Error",message,QMessageBox::Ok,QMessageBox::Ok);
        break;
    case QMessageBox::Information:
        QMessageBox::information(this,"Information",message,QMessageBox::Ok,QMessageBox::Ok);
        break;
    case QMessageBox::Warning:
        QMessageBox::warning(this,"Warning",message,QMessageBox::Ok,QMessageBox::Ok);
        break;
    default:
        QMessageBox::critical(this,"Warning","An error has occurred",QMessageBox::Ok,QMessageBox::Ok);
        break;
    }
}

void BUMain::on_logViewerButton_clicked()
{
    gobLogViewer->setVisible(true);
}

void BUMain::on_cancelButton_clicked()
{
    this->giKeep = 1;
    ui->statusBar->showMessage("Cancelling...");
    emit(main_signal_setStopFlag(1));
    this->gobPaths.clear();
    this->giProgress = 0;
    this->giCopyFileIndex = 0;
}

void BUMain::resetCounters()
{
    this->giTmpTotalFolders = 0;
    this->giTmpTotalFiles = 0;
    this->giTmpTotalFilesSize = 0;
    this->giPathIndex = 0;
    this->giTotalFiles = 0;
    this->giTotalFolders = 0;
    this->giTotalFilesSize = 0;
}

void BUMain::closeEvent(QCloseEvent *event)
{
    if(this->saveSessionToFile("Session.txt"))
        event->accept();
    else
        event->ignore();
}

void BUMain::loadSessionFile(QString asFilePath)
{
    QFile file(asFilePath);
    if(file.exists()){
        ui->fromFilesTextArea->clear();
        if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
            return;

       QTextStream in(&file);
       while (!in.atEnd()) {
          QString line = in.readLine();
          if(!line.startsWith("--")){
              ui->fromFilesTextArea->appendPlainText(line);
          }
          else{
              ui->toFilesTextField->setText(line.replace("--",""));
          }
       }
    }

    this->main_slot_resetCursor();
    this->main_slot_enableFileScan();
}

/**
  Method to validate the source files number, and target.
  Only when the source files number is greater than 0, and
  there is a string in the target line, the backup button
  will be enabled.
*/
void BUMain::checkBackupButton()
{
    if(ui->toFilesTextField->text() != ""
            && !ui->toFilesTextField->text().startsWith(" ")
            && giTotalFiles > 0)
    {

       ui->backupButton->setEnabled(true);
       ui->openTargetButton->setEnabled(true);
    }else{
       ui->backupButton->setEnabled(false);
    }
}

/**
  Method to reset the counters and application state.
*/
void BUMain::resetState()
{
    this->giCurrentPos = -1;
    this->gbCountCancel = true;
    this->ui->overallCopyProgressBar->setMaximum(1);
    this->ui->fileCountLabel->setText("0");
    this->ui->folderCountLabel->setText("0");
    this->ui->totalFileSizeValueLabel->setText("0");
    this->installEventFilters();
    this->main_slot_setStatus("Ready.");
    this->ui->backupButton->setEnabled(false);
}

/**
  Removes all the trailing slashes and backslashes
  of the string passed as argument.
  @param String to crop.
  @return String without trailing slashes.
*/
QString BUMain::removeTrailingSlashes(QString str)
{
    int n = str.size() - 1;
    for (; n >= 0; --n) {
      if (str.at(n) != "/" && str.at(n) != "\\") {
        return str.left(n + 1);
      }
    }
    return "";
}

void BUMain::on_actionQuit_triggered()
{
    this->saveSessionToFile("Session.txt");
    QApplication::quit();
}

void BUMain::on_actionAbout_triggered()
{
    const char *helpText = ("<h2>QBack</h2>"
                            "<style>"
                            "a:link {"
                              "color: orange;"
                              "background-color: transparent;"
                              "text-decoration: none;"
                            "}"
                            "</style>"
                            "<p>Simple but powerful copy utility"
                            "<p> Jaime A. Quiroga P."
                            "<p><a href='https://github.com/GTRONICK/QBack/releases/tag/v1.9.0'>Help Manual for this version</a>"
                            "<p><a href='http://www.gtronick.com'>GTRONICK</a>");

    QMessageBox::about(this, tr("About QBack 1.8.1"),
    tr(helpText));
}

void BUMain::on_actionOpen_session_triggered()
{
    this->main_slot_disableFileScan();
    targetFolder = QFileDialog::getOpenFileName(this, tr("Open Session"),
                                                    "",
                                                    tr("Text files (*.txt);;All files(*.*)"));

    if(targetFolder != NULL && targetFolder != ""){
        loadSessionFile(targetFolder);
    }
}

void BUMain::on_actionLoad_theme_triggered()
{
    targetFolder = QFileDialog::getOpenFileName(this, tr("Open Style"),
                                                    "",
                                                    tr("Stylesheets (*.qss);;All files(*.*)"));

    if(targetFolder != NULL && targetFolder != ""){
        QFile styleFile(targetFolder);
        styleFile.open(QFile::ReadOnly);
        QString StyleSheet = QLatin1String(styleFile.readAll());
        this->setStyleSheet(StyleSheet);
    }
}

void BUMain::on_actionSave_session_triggered()
{
    targetFolder = QFileDialog::getSaveFileName(this, tr("Save session"), "", tr("Text files (*.txt);;All files(*.*)"));

    if(targetFolder != NULL && targetFolder != ""){
        saveSessionToFile(targetFolder);
    }
}

void BUMain::on_actionFind_in_sources_triggered()
{
    gobSearchDialog->setVisible(true);
}

void BUMain::on_actionDefault_theme_triggered()
{
    this->setStyleSheet("");
}

void BUMain::main_slot_processDropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls()){

        QList<QUrl> urlList = mimeData->urls();

        for (int i = 0; i < urlList.size(); i++)
        {
            this->ui->fromFilesTextArea->appendPlainText(urlList.at(i).toLocalFile() + ",");
        }
    }
    event->acceptProposedAction();
}

void BUMain::main_slot_errorOnCopy()
{
    giErrorOnCopyFlag = 1;
}

void BUMain::on_actionEnable_auto_rename_toggled(bool arg1)
{
    emit main_signal_renameEnable(arg1);
}

void BUMain::on_actionInsert_Target_path_triggered()
{
    QTextCursor lobCursor = ui->fromFilesTextArea->textCursor();
    lobCursor.insertText(">" + ui->toFilesTextField->text() + "/");
}
