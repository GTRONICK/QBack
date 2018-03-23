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
    //qDebug() << "Begin BUMain";
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
    gbScanDisabled = false;

    gbIsCtrlPressed = false;
    gsTargetFile = "";

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
    //qDebug() << "End BUMain";
}

/**
 * @brief BUMain::eventFilter
 * Event filter validator. Used whe the user move the mouse over a button.
 * @param obj Button to be filtered.
 * @param event Mouse Enter event.
 * @return True: The mouse entered and is over the object. False otherwise.
 */
bool BUMain::eventFilter(QObject *obj, QEvent *event)
{
    //qDebug() << "Begin eventFilter";
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
    //qDebug() << "End eventFilter";
    return QWidget::eventFilter(obj, event);
}

BUMain::~BUMain()
{
    //qDebug() << "Begin ~BUMain";
    delete ui;
    thread->quit();
    thread->wait();
    //qDebug() << "End ~BUMain";
}

/**
 * @brief BUMain::initThreadSetup
 * Threads and connections setup.
 */
void BUMain::initThreadSetup()
{
    //qDebug() << "Begin initThreadSetup";
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
    //qDebug() << "End initThreadSetup";
}

/**
 * @brief BUMain::installEventFilters
 * Event filters installer. Allow showing the button help in the
 * status bar.
 */
void BUMain::installEventFilters()
{
    //qDebug() << "Begin installEventFilters";
    ui->originButton->installEventFilter(this);
    ui->targetButton->installEventFilter(this);
    ui->logViewerButton->installEventFilter(this);
    ui->openTargetButton->installEventFilter(this);
    ui->backupButton->installEventFilter(this);
    ui->fromFilesTextArea->installEventFilter(this);
    //qDebug() << "End installEventFilters";
}

/**
 * @brief BUMain::uninstallEventFilters
 * Event filter uninstaller. Disable the button help, in the status bar.
 */
void BUMain::uninstallEventFilters()
{
    //qDebug() << "Begin uninstallEventFilters";
    ui->originButton->removeEventFilter(this);
    ui->targetButton->removeEventFilter(this);
    ui->logViewerButton->removeEventFilter(this);
    ui->openTargetButton->removeEventFilter(this);
    ui->backupButton->removeEventFilter(this);
    ui->fromFilesTextArea->removeEventFilter(this);
    //qDebug() << "End uninstallEventFilters";
}

/**
  Save the current session to a text file.
  @param filePath File path to be saved
  @return true if the save was successful, false otherwise.
*/
bool BUMain::saveSessionToFile(QString asFilePath)
{
    //qDebug() << "Begin saveSessionToFile: " << asFilePath;
    QFile file(asFilePath);
    if (!file.open(QIODevice::WriteOnly)){
        //qDebug() << "End saveSessionToFile: " << "false";
        return false;
    }

    QTextStream out(&file);
    out << ui->fromFilesTextArea->toPlainText() + "\n--" + ui->toFilesTextField->text();
    file.close();

    //qDebug() << "End saveSessionToFile: " << "true";
    return true;
}

/**
 * @brief BUMain::on_backupButton_clicked
 * Action triggered when the backup or cancel button is clicked.
 */
void BUMain::on_backupButton_clicked()
{
    //qDebug() << "Begin on_backupButton_clicked";
    this->ui->fromFilesTextArea->setEnabled(false);
    giErrorOnCopyFlag = 0;
    QString lsCurrentPath = "";
    if(gbBackcupButtonPressed == false){

        int recursiveAlertFlag = 0;

        worker->ResetFileCounter();

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

            emit main_signal_logInfo("\n ----- Copy Started ----- \n");
            emit main_signal_setStopFlag(0);
            emit main_signal_logInfo("Creating dirs structure...");

            if(firstPath.contains(">") && firstPath.lastIndexOf(">") != firstPath.length()){
                emit main_signal_createDirs(firstPath.split(">").at(0),firstPath.split(">").at(1),giKeep);
            }else{
                emit main_signal_createDirs(gobPaths.at(0),ui->toFilesTextField->text().trimmed(),giKeep);
            }
        }else{
            this->ui->fromFilesTextArea->setEnabled(true);
            this->main_slot_showMessage("Recursive operation! \nThe target folder contains a source folder",QMessageBox::Critical);
        }
    }else{
        this->ui->fromFilesTextArea->setEnabled(true);
        gbBackcupButtonPressed = false;

        this->ui->backupButton->setText("&Backup!");
        this->ui->backupButton->setIcon(QIcon(":/icons/backupButton.png"));

        emit main_signal_logInfo("\n ----- Copy Cancelled ----- \n");
        on_cancelButton_clicked();
    }

    //qDebug() << "End on_backupButton_clicked";
}

/**
 * @brief BUMain::on_originButton_clicked
 * Action triggered when the origin button is clicked.
 */
void BUMain::on_originButton_clicked()
{
    //qDebug() << "Begin on_originButton_clicked";
    gsTargetFile.clear();
    giTmpTotalFolders = 0;

    dialog->setFileMode(QFileDialog::Directory);
    dialog->setOption(QFileDialog::ShowDirsOnly,true);
    dialog->setOption(QFileDialog::HideNameFilterDetails);
    dialog->setDirectory(gsTargetFile);
    if(dialog->exec()){
        gsTargetFile = dialog->selectedFiles().at(0);
    }

    if(gsTargetFile != NULL && gsTargetFile != ""){
        QDir dir(gsTargetFile);
        dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Hidden | QDir::System);
        dir.setSorting(QDir::Type);
        if(dir.entryList().length() != 0) ui->fromFilesTextArea->clear();
        ui->fromFilesTextArea->appendPlainText(dir.absolutePath() + ",");
    }
    //qDebug() << "End on_originButton_clicked";
}

/**
 * @brief BUMain::on_targetButton_clicked
 * Action triggered when the target button is clicked.
 */
void BUMain::on_targetButton_clicked()
{
    //qDebug() << "Begin on_targetButton_clicked";
    dialog->setFileMode(QFileDialog::Directory);
    dialog->setOption(QFileDialog::ShowDirsOnly,true);
    dialog->setDirectory(ui->toFilesTextField->text());
    if(dialog->exec()){
        gsTargetFile = dialog->selectedFiles().at(0);
    }

    if(gsTargetFile != NULL && gsTargetFile != ""){
        ui->toFilesTextField->setText(gsTargetFile);
    }

    if(giTmpTotalFiles > 0 ) {
        ui->backupButton->setEnabled(true);
    }
    //qDebug() << "End on_targetButton_clicked";
}

/**
 * @brief BUMain::on_toFilesTextField_textChanged
 * Action triggered when the text in the target line, is modified.
 */
void BUMain::on_toFilesTextField_textChanged()
{
    //qDebug() << "Begin on_toFilesTextField_textChanged";
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
    //qDebug() << "End on_toFilesTextField_textChanged";
}

/**
 * @brief BUMain::main_slot_keepCopying
 * Slot triggered when the worker request for a new copy of a single file. This slot
 * is called only one time.
 */
void BUMain::main_slot_keepCopying()
{
    //qDebug() << "Begin main_slot_keepCopying";
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
    //qDebug() << "End main_slot_keepCopying";
}

/**
  Slot triggered when any class need to show a message in the status bar.
  @param Message to be shown.
*/
void BUMain::main_slot_setStatus(QString asStatus)
{
    //qDebug() << "Begin main_slot_setStatus: " << asStatus;
    ui->statusBar->showMessage(asStatus);
    //qDebug() << "End main_slot_setStatus";
}

/**
 * @brief BUMain::main_slot_receiveDirAndFileList
 * Slot triggered whe the worker sends the files to be copied and their destination folders.
 * @param dirs Target directories.
 * @param files Source files to be copied.
 */
void BUMain::main_slot_receiveDirAndFileList(QStringList *dirs, QStringList *files)
{
    //qDebug() << "Begin main_slot_receiveDirAndFileList";
    sourceFiles = files;
    targetDirectories = dirs;
    emit(main_signal_copyFile(sourceFiles->at(giCopyFileIndex),targetDirectories->at(giCopyFileIndex)));
    //qDebug() << "End main_slot_receiveDirAndFileList";
}

/**
 * @brief BUMain::main_slot_copyNextFile
 * Slot triggered when the worker request for a new single file copy. This slot
 * is called for every file to be copied.
 */
void BUMain::main_slot_copyNextFile()
{
    //qDebug() << "Begin main_slot_copyNextFile";
    giCopyFileIndex ++;
    if(giKeep == 0 && giCopyFileIndex < sourceFiles->length()){
        emit(main_signal_copyFile(sourceFiles->at(giCopyFileIndex),targetDirectories->at(giCopyFileIndex)));
    }else{
        this->ui->fromFilesTextArea->setEnabled(true);
        if(giErrorOnCopyFlag == 0) this->main_slot_showMessage("Copy Finished",QMessageBox::Information);
        else this->main_slot_showMessage("Copy Finished with errors! \nSee the log for details.",QMessageBox::Critical);
    }
    //qDebug() << "End main_slot_copyNextFile";
}

/**
 * @brief BUMain::main_slot_resetCursor
 * Slot triggered when the text cursor of the sources text area needs to be reseted.
 */
void BUMain::main_slot_resetCursor()
{
    //qDebug() << "Begin main_slot_resetCursor";
    ui->fromFilesTextArea->moveCursor(QTextCursor::Start);
    //qDebug() << "End main_slot_resetCursor";
}

/**
 * @brief BUMain::main_slot_setTotalFilesAndFolders Adjust the total number of files, folders and size
 * @param aiFileCounter Total number of files.
 * @param aiFolderCounter Total number of folders.
 * @param aiTotalFilesSize Total file size computed.
 */
void BUMain::main_slot_setTotalFilesAndFolders(int aiFileCounter,int aiFolderCounter, qint64 aiTotalFilesSize)
{
    //qDebug() << "Begin main_slot_setTotalFilesAndFolders";
    giTmpTotalFolders = aiFolderCounter;
    giTmpTotalFiles = aiFileCounter;
    giTmpTotalFilesSize = aiTotalFilesSize;

    ui->fileCountLabel->setText(aiFileCounter > 0 ? QString::number(aiFileCounter):QString::number(0));
    ui->folderCountLabel->setText(aiFolderCounter > 0 ? QString::number(aiFolderCounter):QString::number(0));
    ui->totalFileSizeValueLabel->setText(aiTotalFilesSize > 0 ? QString::number(aiTotalFilesSize):QString::number(0));
    //qDebug() << "End main_slot_setTotalFilesAndFolders";
}

/**
 * @brief BUMain::main_slot_workerDone
 * Slot triggered wheb the counting of files and folders ends.
 */
void BUMain::main_slot_workerDone()
{
    //qDebug() << "Begin main_slot_workerDone";
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
    //qDebug() << "End main_slot_workerDone";
}

/**
 * @brief BUMain::main_slot_getTextEdit
 * Slot to send the text to the worker
 */
void BUMain::main_slot_getTextEdit()
{
    //qDebug() << "Begin main_slot_getTextEdit";
    emit main_signal_setTextEdit(this->ui->fromFilesTextArea);
    //qDebug() << "End main_slot_getTextEdit";
}

/**
 * @brief BUMain::on_openTargetButton_clicked
 * Slot triggered when a click on the Target Button, is detected.
 */
void BUMain::on_openTargetButton_clicked()
{
    //qDebug() << "Begin on_openTargetButton_clicked";
    ui->toFilesTextField->setText(ui->toFilesTextField->text().trimmed());

    if(ui->toFilesTextField->text() != NULL &&
            ui->toFilesTextField->text() != "" &&
            !ui->toFilesTextField->text().startsWith(' ')) {
        if(!gbIsCtrlPressed) {
            if(!QDesktopServices::openUrl(QUrl::fromLocalFile(ui->toFilesTextField->text()))) {
                this->main_slot_showMessage("Target folder \"" + ui->toFilesTextField->text() + "\" not found!",QMessageBox::Critical);
            }
        } else {
            QString lsFisrtOrigin = ui->fromFilesTextArea->toPlainText();

            lsFisrtOrigin = lsFisrtOrigin.split(",").at(0);

            if(QFile(lsFisrtOrigin).exists() && QFileInfo(lsFisrtOrigin).isDir()) {
                ui->statusBar->showMessage("Opening origin folder...",1000);
                if(!QDesktopServices::openUrl(QUrl::fromLocalFile(lsFisrtOrigin))) {
                    this->main_slot_showMessage("Origin folder \"" + ui->toFilesTextField->text() + "\" not found!",QMessageBox::Critical);
                } else {
                    gbIsCtrlPressed = false;
                    ui->openTargetButton->setText("Open Target");
                }
            } else {
                ui->statusBar->showMessage("The first path is not a folder!",1000);
            }
        }
    }
    //qDebug() << "End on_openTargetButton_clicked";
}

/**
 * @brief BUMain::on_fromFilesTextArea_textChanged
 * Slot triggered when the text in the source files text area, is altered.
 */
void BUMain::on_fromFilesTextArea_textChanged()
{
    //qDebug() << "Begin on_fromFilesTextArea_textChanged";
    if(gbScanDisabled == false){
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
                gbCountCancel = false;
                emit main_signal_scanFolders(gobPaths.at(giPathIndex));
            }else{
                resetState();
            }

        }else if(lsAreaText.lastIndexOf(",") == -1){

            resetState();
        }
    }
    //qDebug() << "End on_fromFilesTextArea_textChanged";
}

/**
 * @brief BUMain::main_slot_scanReady
 */
void BUMain::main_slot_scanReady()
{
    //qDebug() << "Begin main_slot_scanReady";
    if(gbCountCancel == false ){
        emit main_signal_scanNextPath();
    }
    //qDebug() << "End main_slot_scanReady";
}

/**
 * @brief BUMain::main_slot_disableFileScan
 * Slot used to disable the file counting while the sources text area is modified, or when files
 * are dropped.
 */
void BUMain::main_slot_disableFileScan()
{
    //qDebug() << "Begin main_slot_disableFileScan";
    gbScanDisabled = true;
    //qDebug() << "End main_slot_disableFileScan";
}

/**
 * @brief BUMain::main_slot_enableFileScan
 * Slot used to diable the file counting, after modify the sources text area, or drop files.
 */
void BUMain::main_slot_enableFileScan()
{
    //qDebug() << "Begin main_slot_enableFileScan";
    gbScanDisabled = false;
    this->on_fromFilesTextArea_textChanged();
    //qDebug() << "End main_slot_enableFileScan";
}

/**
 * @brief BUMain::main_slot_showMessage
 * Slot used to show messages in the screen. Mainly used by the worker Thread.
 * @param message
 * @param messageType
 */
void BUMain::main_slot_showMessage(QString message, int messageType)
{
    //qDebug() << "Begin main_slot_showMessage";
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
        QMessageBox::critical(this,"Warning","An error has occurred, check the log for details.",QMessageBox::Ok,QMessageBox::Ok);
        break;
    }
    //qDebug() << "End main_slot_showMessage";
}

/**
 * @brief BUMain::on_logViewerButton_clicked
 * Slot triggered when the user clicks the Log viewer button.
 */
void BUMain::on_logViewerButton_clicked()
{
    //qDebug() << "Begin on_logViewerButton_clicked";
    gobLogViewer->setVisible(true);
    //qDebug() << "End on_logViewerButton_clicked";
}

/**
 * @brief BUMain::on_cancelButton_clicked
 * Slot triggered when the user clicks the Cancel button.
 */
void BUMain::on_cancelButton_clicked()
{
    //qDebug() << "Begin on_cancelButton_clicked";
    this->giKeep = 1;
    ui->statusBar->showMessage("Cancelling...");
    emit(main_signal_setStopFlag(1));
    this->gobPaths.clear();
    this->giProgress = 0;
    this->giCopyFileIndex = 0;
    //qDebug() << "End on_cancelButton_clicked";
}

/**
 * @brief BUMain::resetCounters
 * Function used to reset the global counters, used in file counting, and copying.
 */
void BUMain::resetCounters()
{
    //qDebug() << "Begin resetCounters";
    this->giTmpTotalFolders = 0;
    this->giTmpTotalFiles = 0;
    this->giTmpTotalFilesSize = 0;
    this->giPathIndex = 0;
    this->giTotalFiles = 0;
    this->giTotalFolders = 0;
    this->giTotalFilesSize = 0;
    //qDebug() << "End resetCounters";
}

/**
 * @brief BUMain::closeEvent
 * Function invoked when the user closes the main window.
 * @param event Close event.
 */
void BUMain::closeEvent(QCloseEvent *event)
{
    //qDebug() << "Begin closeEvent";
    if(this->saveSessionToFile("Session.txt"))
        event->accept();
    else
        event->ignore();
    //qDebug() << "End closeEvent";
}

/**
 * @brief BUMain::loadSessionFile
 *
 * @param asFilePath
 */
void BUMain::loadSessionFile(QString asFilePath)
{
    //qDebug() << "Begin loadSessionFile: " << asFilePath;
    this->main_slot_disableFileScan();
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
    //qDebug() << "End loadSessionFile";
}

/**
 * @brief BUMain::checkBackupButton
 * Method to validate the source files number, and target.
 * Only when the source files number is greater than 0, and
 * there is a string in the target line, the backup button
 * will be enabled.
 */
void BUMain::checkBackupButton()
{
    //qDebug() << "Begin checkBackupButton";
    if(ui->toFilesTextField->text() != ""
            && !ui->toFilesTextField->text().startsWith(" ")
            && giTotalFiles > 0)
    {

       ui->backupButton->setEnabled(true);
       ui->openTargetButton->setEnabled(true);
    }else{
       ui->backupButton->setEnabled(false);
    }
    //qDebug() << "End checkBackupButton";
}

/**
 * @brief BUMain::resetState
 * Function used to reset the counters and application state, in the graphical window.
 */
void BUMain::resetState()
{
    //qDebug() << "Begin resetState";
    this->giCurrentPos = -1;
    this->gbCountCancel = true;
    this->ui->overallCopyProgressBar->setMaximum(1);
    this->ui->fileCountLabel->setText("0");
    this->ui->folderCountLabel->setText("0");
    this->ui->totalFileSizeValueLabel->setText("0");
    this->installEventFilters();
    this->main_slot_setStatus("Ready.");
    this->ui->backupButton->setEnabled(false);
    //qDebug() << "End resetState";
}

/**
  Removes all the trailing slashes and backslashes
  of the string passed as argument.
  @param String to crop.
  @return String without trailing slashes.
*/
QString BUMain::removeTrailingSlashes(QString lsString)
{
    //qDebug() << "Begin removeTrailingSlashes: " << lsString;
    int n = lsString.size() - 1;
    for (; n >= 0; --n) {
        if (lsString.at(n) != "/" && lsString.at(n) != "\\") {
            //qDebug() << "End removeTrailingSlashes: " << lsString.left(n + 1);
            return lsString.left(n + 1);
        }
    }
    //qDebug() << "End removeTrailingSlashes: "
    return "";
}

/**
 * @brief BUMain::on_actionQuit_triggered
 * Slot invoked when the user closes the application, via File -> Quit.
 */
void BUMain::on_actionQuit_triggered()
{
    //qDebug() << "Begin on_actionQuit_triggered"
    this->saveSessionToFile("Session.txt");
    QApplication::quit();
    //qDebug() << "End on_actionQuit_triggered"
}

/**
 * @brief BUMain::on_actionAbout_triggered
 * Slot triggered when the user clicks on About QBack. Shows basic application information,
 * and provides a link to go to the release page.
 */
void BUMain::on_actionAbout_triggered()
{
    //qDebug() << "Begin on_actionAbout_triggered"
    const char *helpText = ("<h2>QBack</h2>"
                            "<style>"
                            "a:link {"
                              "color: orange;"
                              "background-color: transparent;"
                              "text-decoration: none;"
                            "}"
                            "</style>"
                            "<p>Simple but powerful copy utility"
                            "<p>Jaime A. Quiroga P."
                            "<p><a href='https://github.com/GTRONICK/QBack/releases/tag/v1.9.0'>Help for this version</a>"
                            "<p><a href='http://www.gtronick.com'>GTRONICK</a>");

    QMessageBox::about(this, tr("About QBack 1.9.1"),
    tr(helpText));
    //qDebug() << "End on_actionAbout_triggered"
}

/**
 * @brief BUMain::on_actionOpen_session_triggered
 * Slot used to load an especific session. This will load the files, and target
 * where the files will be copied. The file counter, folder counter, and
 * total file size will update accordingly.
 */
void BUMain::on_actionOpen_session_triggered()
{
    //qDebug() << "Begin on_actionOpen_session_triggered"
    this->main_slot_disableFileScan();
    gsTargetFile = QFileDialog::getOpenFileName(this, tr("Open Session"),
                                                    "",
                                                    tr("Text files (*.txt);;All files(*.*)"));

    if(gsTargetFile != NULL && gsTargetFile != ""){
        loadSessionFile(gsTargetFile);
    }
    //qDebug() << "End on_actionOpen_session_triggered"
}

/**
 * @brief BUMain::on_actionLoad_theme_triggered
 * Slot used to load an especific theme for the application.
 * The file could be any text file with a valid Style sheet.
 */
void BUMain::on_actionLoad_theme_triggered()
{
    //qDebug() << "Begin on_actionLoad_theme_triggered"
    gsTargetFile = QFileDialog::getOpenFileName(this, tr("Open Style"),
                                                    "",
                                                    tr("Stylesheets (*.qss);;All files(*.*)"));

    if(gsTargetFile != NULL && gsTargetFile != ""){
        QFile styleFile(gsTargetFile);
        styleFile.open(QFile::ReadOnly);
        QString StyleSheet = QLatin1String(styleFile.readAll());
        this->setStyleSheet(StyleSheet);
    }
    //qDebug() << "End on_actionLoad_theme_triggered"
}

/**
 * @brief BUMain::on_actionSave_session_triggered
 * Slot used to save the current session, to a text file.
 * This will save the source files, and the target folder.
 */
void BUMain::on_actionSave_session_triggered()
{
    //qDebug() << "Begin on_actionSave_session_triggered"

    gsTargetFile = QFileDialog::getSaveFileName(this, tr("Save session"), ui->toFilesTextField->text().trimmed(), tr("Text files (*.txt);;All files(*.*)"));

    if(gsTargetFile != NULL && !gsTargetFile.isEmpty()){
        ui->statusBar->showMessage("Saving current session to file...",1000);
        saveSessionToFile(gsTargetFile);
    }


    //qDebug() << "End on_actionSave_session_triggered"
}

/**
 * @brief BUMain::on_actionFind_in_sources_triggered
 * Slot used to show the Search & Replace dialog.
 */
void BUMain::on_actionFind_in_sources_triggered()
{
    //qDebug() << "Begin on_actionFind_in_sources_triggered"
    gobSearchDialog->setVisible(true);
    //qDebug() << "End on_actionFind_in_sources_triggered"
}

/**
 * @brief BUMain::on_actionDefault_theme_triggered
 * Slot used to reset the theme application.
 */
void BUMain::on_actionDefault_theme_triggered()
{
    //qDebug() << "Begin on_actionDefault_theme_triggered"
    this->setStyleSheet("");
    //qDebug() << "End on_actionDefault_theme_triggered"
}

/**
 * @brief BUMain::main_slot_processDropEvent
 * Slot called when files are dropped in the sources text area.
 * @param event Drop event.
 */
void BUMain::main_slot_processDropEvent(QDropEvent *event)
{
    //qDebug() << "Begin main_slot_processDropEvent"
    this->main_slot_disableFileScan();
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls()){

        QList<QUrl> urlList = mimeData->urls();

        for (int i = 0; i < urlList.size(); i++)
        {
            this->ui->fromFilesTextArea->appendPlainText(urlList.at(i).toLocalFile() + ",");
        }
    }
    event->acceptProposedAction();
    this->main_slot_enableFileScan();
    //qDebug() << "Begin main_slot_processDropEvent"
}

/**
 * @brief BUMain::main_slot_errorOnCopy
 * Slot used to indicate a copy failure, using a flag.
 */
void BUMain::main_slot_errorOnCopy()
{
    //qDebug() << "Begin main_slot_errorOnCopy"
    giErrorOnCopyFlag = 1;
    //qDebug() << "End main_slot_errorOnCopy"
}

/**
 * @brief BUMain::on_actionEnable_auto_rename_toggled
 * Slot used to tell to the worker, that the Automatic Rename, has been enabled.
 * @param arg1 True: Rename enabled. False otherwise.
 */
void BUMain::on_actionEnable_auto_rename_toggled(bool arg1)
{
    //qDebug() << "Begin on_actionEnable_auto_rename_toggled: " << arg1
    emit main_signal_renameEnable(arg1);
    //qDebug() << "End on_actionEnable_auto_rename_toggled: " << arg1
}

/**
 * @brief BUMain::on_actionInsert_Target_path_triggered
 * Slot used to insert the target path, at the current cursor position.
 * This will append a > and a / to the path, letting it ready for
 * modification.
 */
void BUMain::on_actionInsert_Target_path_triggered()
{
    //qDebug() << "Begin on_actionInsert_Target_path_triggered"
    QTextCursor lobCursor = ui->fromFilesTextArea->textCursor();
    lobCursor.insertText(">" + ui->toFilesTextField->text() + "/");
    //qDebug() << "End on_actionInsert_Target_path_triggered"
}

void BUMain::keyPressEvent(QKeyEvent *event)
{
    //qDebug() << QString::number(event->nativeScanCode());
    switch (event->nativeScanCode()) {
    case 29: // left ctrl
        gbIsCtrlPressed = true;
        ui->openTargetButton->setText("Open &Origin");
        break;
    }
}

void BUMain::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->nativeScanCode()) {
    case 29: // left ctrl
        gbIsCtrlPressed = false;
        ui->openTargetButton->setText("Open &Target");
        break;
    }
}

