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

BUMain::BUMain(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::BUMain)
{

    ui->setupUi(this);

    QFile styleFile("style.qss");
    styleFile.open(QFile::ReadOnly);
    QString StyleSheet = QLatin1String(styleFile.readAll());

    dialog = new QFileDialog(this);
    gobLogViewer = new LogViewer;
    gobSearchDialog = new SearchDialog(this);
    targetDirectories = new QStringList;
    sourceFiles = new QStringList;

    giCurrentPos = -1;
    giProgress = 0;

    giKeep = 0;
    giCopyFileIndex = 0;
    validatorFlag = 0;
    giFlag = 0;
    giControl = 0;

    gbBackcupButtonPressed = false;
    gbCountCancel = false;

    this->resetCounters();
    this->setStyleSheet(StyleSheet);
    this->installEventFilters();
    this->move(QApplication::desktop()->screen()->rect().center() - this->rect().center());
    this->ui->backupButton->setEnabled(false);
    this->ui->openTargetButton->setEnabled(false);
    this->initThreadSetup();
    this->loadSessionFile("Session.qbs");
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
    connect(worker,SIGNAL(worker_signal_showMessage(QString)),this,SLOT(main_slot_showMessage(QString)));
    connect(this,SIGNAL(main_signal_setStopFlag(int)),worker,SLOT(worker_slot_setStopFlag(int)));
    connect(this,SIGNAL(main_signal_readyToStartCopy()),worker,SLOT(worker_slot_readyToStartCopy()));
    connect(worker,SIGNAL(worker_signal_sendDirAndFileList(QStringList*,QStringList*)),this,SLOT(main_slot_receiveDirAndFileList(QStringList*,QStringList*)));
    connect(this,SIGNAL(main_signal_copyFile(QString,QString)),worker,SLOT(worker_slot_copyFile(QString,QString)));
    connect(worker,SIGNAL(worker_signal_copyNextFile()),this,SLOT(main_slot_copyNextFile()));
    connect(this,SIGNAL(main_signal_logInfo(QString)),gobLogViewer,SLOT(logger_slot_logInfo(QString)));
    connect(gobSearchDialog,SIGNAL(search_signal_resetCursor()),this,SLOT(main_slot_resetCursor()));
    connect(this,SIGNAL(main_signal_scanFolders(QString)),worker,SLOT(worker_slot_scanFolders(QString)));
    connect(worker,SIGNAL(worker_signal_setTotalFilesAndFolders(int,int,qint64)),this,SLOT(main_slot_setTotalFilesAndFolders(int,int,qint64)));
    connect(worker,SIGNAL(worker_signal_workerDone()), this,SLOT(main_slot_workerDone()));
    connect(gobSearchDialog,SIGNAL(search_signal_getTextEditText()),this,SLOT(main_slot_getTextEdit()));
    connect(this,SIGNAL(main_signal_setTextEdit(QPlainTextEdit*)),gobSearchDialog,SLOT(search_slot_setTextEdit(QPlainTextEdit*)));
    connect(worker,SIGNAL(worker_signal_scanReady()),this,SLOT(main_slot_scanReady()));
    connect(this,SIGNAL(main_signal_scanNextPath()),worker,SLOT(worker_slot_scanNextPath()));
    connect(thread,SIGNAL(finished()),worker,SLOT(deleteLater()));
    connect(thread,SIGNAL(finished()),thread,SLOT(deleteLater()));
    thread->start();
}

void BUMain::installEventFilters()
{
    ui->originButton->installEventFilter(this);
    ui->targetButton->installEventFilter(this);
    ui->logViewerButton->installEventFilter(this);
    ui->openTargetButton->installEventFilter(this);
    ui->backupButton->installEventFilter(this);
    ui->fromFilesTextArea->installEventFilter(this);
}

void BUMain::uninstallEventFilters()
{
    ui->originButton->removeEventFilter(this);
    ui->targetButton->removeEventFilter(this);
    ui->logViewerButton->removeEventFilter(this);
    ui->openTargetButton->removeEventFilter(this);
    ui->backupButton->removeEventFilter(this);
    ui->fromFilesTextArea->removeEventFilter(this);
}

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

void BUMain::on_backupButton_clicked()
{
    if(gbBackcupButtonPressed == false){

        int recursiveAlertFlag = 0;

        worker->setFileCounter(0);

        gobPaths = ui->fromFilesTextArea->toPlainText().split(",");
        gobPaths.removeAt(gobPaths.length() - 1);
        for(int i = 0; i < gobPaths.length(); i++){
            gobPaths.replace(i,gobPaths.at(i).trimmed());
            if(ui->toFilesTextField->text().contains(gobPaths.at(i))){
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
                qDebug() << "Custom path found";
                emit(main_signal_createDirs(firstPath.split(">").at(0),firstPath.split(">").at(1),giKeep));
            }else{
                emit(main_signal_createDirs(gobPaths.at(0),ui->toFilesTextField->text().trimmed(),giKeep));
            }
        }else{
            QMessageBox::critical(this,"Recursive operation alert!","The target folder contains a source folder");
        }
    }else{

        gbBackcupButtonPressed = false;

        this->ui->backupButton->setText("&Backup!");
        this->ui->backupButton->setIcon(QIcon(":/icons/backupButton.png"));

        emit(main_signal_logInfo("\n ----- Copy Cancelled ----- \n"));
        on_cancelButton_clicked();
    }
}

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
        // qDebug() << "main: Scan finished, emmiting SIGNAL readyToStartCopy";
        emit(main_signal_readyToStartCopy());
        ui->statusBar->showMessage("Ready.");
    }
}

void BUMain::main_slot_setStatus(QString status)
{
    ui->statusBar->showMessage(status);
}

void BUMain::main_slot_receiveDirAndFileList(QStringList *dirs, QStringList *files)
{
    sourceFiles = files;
    targetDirectories = dirs;
    // qDebug() << "main: Emmiting signal copyFile with " << sourceFiles->at(giCopyFileIndex) << ", " << targetDirectories->at(giCopyFileIndex);
    emit(main_signal_copyFile(sourceFiles->at(giCopyFileIndex),targetDirectories->at(giCopyFileIndex)));

}

void BUMain::main_slot_copyNextFile()
{
    // qDebug() << "main: copyNextFile SIGNAL received ";
    giCopyFileIndex ++;
    if(giKeep == 0 && giCopyFileIndex < sourceFiles->length()){
        // qDebug() << "main: Emmiting SIGNAL copyFile with " << sourceFiles->at(giCopyFileIndex) << ", " << targetDirectories->at(giCopyFileIndex);
        emit(main_signal_copyFile(sourceFiles->at(giCopyFileIndex),targetDirectories->at(giCopyFileIndex)));
    }else{
        // qDebug() << "Copy finished.";
        gbBackcupButtonPressed = false;
        this->ui->backupButton->setText("&Backup!");
        this->ui->backupButton->setIcon(QIcon(":/icons/backupButton.png"));
    }
}

void BUMain::main_slot_resetCursor()
{
    // qDebug() << "main: resetCursor SIGNAL Recevied";
    ui->fromFilesTextArea->moveCursor(QTextCursor::Start);
}

void BUMain::main_slot_setTotalFilesAndFolders(int aiFileCounter,int aiFolderCounter, qint64 aiTotalFilesSize)
{
    if(giFlag == 1){
        giTmpTotalFolders = aiFolderCounter;
        giTmpTotalFiles = aiFileCounter;
        giTmpTotalFilesSize = aiTotalFilesSize;

        ui->fileCountLabel->setText(aiFileCounter > 0 ? QString::number(aiFileCounter):QString::number(0));
        ui->folderCountLabel->setText(aiFolderCounter > 0 ? QString::number(aiFolderCounter):QString::number(0));
        ui->totalFileSizeValueLabel->setText(aiTotalFilesSize > 0 ? QString::number(aiTotalFilesSize):QString::number(0));
    }
}

void BUMain::main_slot_workerDone()
{
    if(giFlag == 1){

//    qDebug() << "main: SIGNAL workerDone received";
        giPathIndex ++;
        giTotalFiles += giTmpTotalFiles;
        giTotalFilesSize += giTmpTotalFilesSize;
        giTotalFolders += giTmpTotalFolders;
        if(giPathIndex < gobPaths.length()){
            emit(main_signal_scanFolders(gobPaths.at(giPathIndex)));

        }else{
            this->installEventFilters();
            ui->fileCountLabel->setText(giTotalFiles > 0 ? QString::number(giTotalFiles):QString::number(0));
            ui->folderCountLabel->setText(giTotalFolders > 0 ? QString::number(giTotalFolders):QString::number(0));
            ui->totalFileSizeValueLabel->setText(giTotalFilesSize > 0 ? QString::number(giTotalFilesSize):QString::number(0));
            ui->overallCopyProgressBar->setMaximum(giTotalFiles > 0 ? giTotalFiles : 1);
            checkBackupButton();
            //resetCounters();
        }
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
            QMessageBox::critical(this,"Error","Target folder \"" + ui->toFilesTextField->text() + "\" not found!");
        }
    }
}

void BUMain::on_fromFilesTextArea_textChanged()
{

    resetCounters();

    QString lsAreaText = ui->fromFilesTextArea->toPlainText();

    if(lsAreaText.lastIndexOf(",") != giCurrentPos && lsAreaText.lastIndexOf(",") != -1 && giFlag == 1){

        ui->overallCopyProgressBar->setMaximum(0);
        // qDebug() << "main: Comma detected!";
        gobPaths.clear();
        gbCountCancel = false;
        giCurrentPos = lsAreaText.lastIndexOf(",");
        gobPaths = lsAreaText.split(",");
        gobPaths.removeAt(gobPaths.length() - 1);
        this->uninstallEventFilters();
        emit(main_signal_scanFolders(gobPaths.at(giPathIndex)));

    }else if(lsAreaText.lastIndexOf(",") == -1){
        resetCounters();
        giCurrentPos = -1;
        gbCountCancel = true;
        this->ui->overallCopyProgressBar->setMaximum(1);
        this->ui->fileCountLabel->setText("0");
        this->ui->folderCountLabel->setText("0");
        this->ui->totalFileSizeValueLabel->setText("0");
        this->installEventFilters();
        // qDebug() << "Calling workerDone from main";
        this->main_slot_setStatus("Ready.");
        ui->backupButton->setEnabled(false);
    }
}

void BUMain::main_slot_scanReady()
{

    // qDebug() << "main: scanReady SIGNAL received";
    if(gbCountCancel == false ){
        // qDebug() << "main: Emitting scanNextPath SIGNAL";
        emit(main_signal_scanNextPath());
    }
}

void BUMain::on_logViewerButton_clicked()
{
    gobLogViewer->setVisible(true);
}

void BUMain::main_slot_showMessage(QString message)
{
    QMessageBox::critical(this,"Error",message,QMessageBox::Ok,QMessageBox::Ok);
    this->ui->backupButton->setText("&Backup!");
    this->ui->backupButton->setIcon(QIcon(":/icons/backupButton.png"));
    gbBackcupButtonPressed = false;
    giCopyFileIndex = 0;
}

void BUMain::on_cancelButton_clicked()
{

    giKeep = 1;
    ui->statusBar->showMessage("Cancelling...");
    emit(main_signal_setStopFlag(1));
    gobPaths.clear();
    giProgress = 0;
    giCopyFileIndex = 0;
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
    if(this->saveSessionToFile("Session.qbs"))
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

       resetCounters();
    }

    giFlag = 1;
    this->main_slot_resetCursor();

    this->on_fromFilesTextArea_textChanged();
}

void BUMain::on_actionQuit_triggered()
{
    this->saveSessionToFile("Session.qbs");
    QApplication::quit();
}

void BUMain::on_actionAbout_triggered()
{
    const char *helpText = ("<h2>QBack</h2>"
               "<p>Enter each file path ended with comma ( , ) and without trailing spaces."
               "For example:"
               "<p>C:\\File.txt,"
               "<br>C:\\Documents and settings\\Document.pdf,"
               "<br>/home/user/Documents/script.sh,"
               "<p>Use the greater than symbol ( > ), to copy to a different target, for example:"
               "<br>/home/user/myTextFyle.txt>/media/USB/Backup,"
               "<p>You can paste clipboard contents here, but be sure to end each file path with comma");

    QMessageBox::about(this, tr("About QBack 1.5.0"),
    tr(helpText));
}

void BUMain::on_actionOpen_session_triggered()
{
    giFlag = 0;
    targetFolder = QFileDialog::getOpenFileName(this, tr("Open Session"),
                                                    "",
                                                    tr("Session files (*.qbs);;All files(*.*)"));

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

    targetFolder = QFileDialog::getSaveFileName(this, tr("Save session"), "", tr("Session files (*.qbs);;All files(*.*)"));

    if(targetFolder != NULL && targetFolder != ""){
        saveSessionToFile(targetFolder);
    }
}

void BUMain::on_actionFind_in_sources_triggered()
{
    // qDebug() << "Find pressed";
    gobSearchDialog->setVisible(true);
}

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

void BUMain::on_actionDefault_theme_triggered()
{
    this->setStyleSheet("");
}
