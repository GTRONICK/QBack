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
    ui->statusBar->showMessage("Version: " + QString::number(APP_VERSION),0);
    this->installEventFilters();
    ui->backupButton->setEnabled(false);
    ui->openTargetButton->setEnabled(false);
    dialog = new QFileDialog(this);
    giCurrentPos = 0;
    giProgress = 0;
    giFileCounter = 0;
    giTotalFolders = 0;
    validatorFlag = 0;
    giKeep = 0;
    gobLogViewer = new LogViewer;
    targetDirectories = new QStringList;
    sourceFiles = new QStringList;
    giCopyFileIndex = 0;

    this->initThreadSetup();
    this->move(QApplication::desktop()->screen()->rect().center() - this->rect().center());
    this->loadSessionFile("Session.txt");

    QFile styleFile("style.qss");
    styleFile.open(QFile::ReadOnly);
    QString StyleSheet = QLatin1String(styleFile.readAll());
    this->setStyleSheet(StyleSheet);
}

bool BUMain::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == (QObject*)ui->originButton) {
        if (event->type() == QEvent::Enter){
            ui->statusBar->showMessage("Select the files to copy",TOOLTIP_DURATION);
        }
    }else if(obj == (QObject*)ui->helpButton){
        if (event->type() == QEvent::Enter){
            ui->statusBar->showMessage("Show help dialog",TOOLTIP_DURATION);
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
    }else if(obj == (QObject*)ui->cancelButton){
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
    connect(worker,SIGNAL(worker_Signal_updateProgressBar(int)),ui->overallCopyProgressBar,SLOT(setValue(int)),Qt::QueuedConnection);
    connect(this,SIGNAL(main_signal_createDirs(QString,QString,int)),worker,SLOT(worker_slot_createDirs(QString,QString,int)),Qt::QueuedConnection);
    connect(worker,SIGNAL(worker_signal_keepCopying()),this,SLOT(main_slot_keepCopying()),Qt::QueuedConnection);
    connect(worker,SIGNAL(worker_signal_logInfo(QString)),gobLogViewer,SLOT(logger_slot_logInfo(QString)),Qt::QueuedConnection);
    connect(worker,SIGNAL(worker_signal_statusInfo(QString)),this,SLOT(main_slot_setStatus(QString)));
    connect(worker,SIGNAL(worker_signal_showMessage(QString)),this,SLOT(main_slot_showMessage(QString)));
    connect(this,SIGNAL(main_signal_setStopFlag(int)),worker,SLOT(worker_slot_setStopFlag(int)));
    connect(this,SIGNAL(main_signal_readyToStartCopy()),worker,SLOT(worker_slot_readyToStartCopy()));
    connect(worker,SIGNAL(worker_signal_sendDirAndFileList(QStringList*,QStringList*)),this,SLOT(main_slot_receiveDirAndFileList(QStringList*,QStringList*)));
    connect(this,SIGNAL(main_signal_copyFile(QString,QString)),worker,SLOT(worker_slot_copyFile(QString,QString)));
    connect(worker,SIGNAL(worker_signal_copyNextFile()),this,SLOT(main_slot_copyNextFile()));
    connect(thread,SIGNAL(finished()),worker,SLOT(deleteLater()));
    connect(thread,SIGNAL(finished()),thread,SLOT(deleteLater()));
    thread->start();

}

int BUMain::countAllFiles(QString path)
{
    int total = 0;

    QFileInfo source(path);
    if(source.isDir()){
        giTotalFolders ++;
        QDir lobDir(path);
        lobDir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        QStringList files = lobDir.entryList();

        total = lobDir.count();
        for(int index = 0; index < files.length(); index++){
            total += countAllFiles(lobDir.absolutePath() + QLatin1Char('/') + files.at(index));
        }
    }

    return total;
}

void BUMain::installEventFilters()
{
    ui->originButton->installEventFilter(this);
    ui->helpButton->installEventFilter(this);
    ui->targetButton->installEventFilter(this);
    ui->logViewerButton->installEventFilter(this);
    ui->openTargetButton->installEventFilter(this);
    ui->backupButton->installEventFilter(this);
    ui->fromFilesTextArea->installEventFilter(this);
    ui->cancelButton->installEventFilter(this);
}

void BUMain::uninstallEventFilters()
{
    ui->originButton->removeEventFilter(this);
    ui->helpButton->removeEventFilter(this);
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
    emit(main_signal_setStopFlag(0));
    giKeep = 0;
    ui->toFilesTextField->setText(ui->toFilesTextField->text().trimmed());
    giCopyFileIndex = 0;
    this->uninstallEventFilters();
    giProgress = 0;
    int recursiveAlertFlag = 0;

    worker->setFileCounter(0);

    gobPaths = ui->fromFilesTextArea->toPlainText().split(",");
    gobPaths.removeAt(gobPaths.length() - 1);
    for(int i = 0; i < gobPaths.length(); i++){
        gobPaths.replace(i,gobPaths.at(i).trimmed());
        if(gobPaths.at(i) == ui->toFilesTextField->text().trimmed()){
            recursiveAlertFlag = 1;
            break;
        }
    }

    if(recursiveAlertFlag == 0){
        ui->overallCopyProgressBar->setValue(0);
        ui->overallCopyProgressBar->setMaximum(giFileCounter > 0 ? giFileCounter:1);
        emit(main_signal_createDirs(gobPaths.at(0),ui->toFilesTextField->text().trimmed(),giKeep));
    }else{
        QMessageBox::critical(this,"Recursive operation alert!","The target folder is the same source folder");
    }
}

void BUMain::on_originButton_clicked()
{
    targetFolder.clear();
    giTotalFolders = 0;
    giTotalFiles = 0;

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

    if(targetFolder != NULL && targetFolder != "" && giFileCounter > 0){
        ui->backupButton->setEnabled(true);
    }

    ui->toFilesTextField->setText(targetFolder);
}

void BUMain::on_toFilesTextField_textChanged()
{
    validatorFlag = 0;
    if(ui->toFilesTextField->text() != NULL &&
            ui->toFilesTextField->text() != "" &&
            !ui->toFilesTextField->text().startsWith(" ")){
        validatorFlag = 1;
        if(giFileCounter > 0){
            ui->backupButton->setEnabled(true);
        }

        ui->openTargetButton->setEnabled(true);

    }else{
        ui->backupButton->setEnabled(false);
        ui->openTargetButton->setEnabled(false);
    }
}

void BUMain::on_helpButton_clicked()
{
    const char *helpText = ("<h2>QBack</h2>"
               "<p>Copyright &copy; 2016 GTRONICK."
               "<p>Enter each file path ended with comma ( , ) and without trailing spaces."
               "For example:"
               "<p>C:\\File.txt,"
               "<br>C:\\Documents and settings\\Document.pdf,"
               "<br>/home/user/Documents/script.sh,"
               "<p>You can paste clipboard contents here, but be sure to end each file path with comma");

    QMessageBox::about(this, tr("About Backup Utility"),
    tr(helpText));
}

void BUMain::main_slot_keepCopying()
{
    giProgress ++;
    if(giProgress < gobPaths.length() && giKeep == 0){
        emit(main_signal_createDirs(gobPaths.at(giProgress),ui->toFilesTextField->text(),giKeep));
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

    // qDebug() << "main: sendDirAndFileList SIGNAL received, printing lists... ";
    // qDebug() << "Directories: ";
    for(int i = 0; i < targetDirectories->length(); i++){
        // qDebug() << targetDirectories->at(i);
    }

    // qDebug() << "Files: ";
    for(int i = 0; i < sourceFiles->length(); i++){
        // qDebug() << sourceFiles->at(i);
    }

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
    }
}

void BUMain::on_openTargetButton_clicked()
{

    ui->toFilesTextField->setText(ui->toFilesTextField->text().trimmed());

    if(ui->toFilesTextField->text() != NULL &&
            ui->toFilesTextField->text() != "" &&
            !ui->toFilesTextField->text().startsWith(' ')){

        if(!QDesktopServices::openUrl(ui->toFilesTextField->text().replace("\\","/"))){
            QMessageBox::critical(this,"Error","Target folder \"" + ui->toFilesTextField->text() + "\" not found!");
        }
    }
}

void BUMain::on_fromFilesTextArea_textChanged()
{
    ui->overallCopyProgressBar->setValue(0);

    giTotalFolders = 0;
    int index = 0;
    int fileCount = 0;
    int notFolderCount = 0;
    int counted = 0;


    QString lsAreaText = ui->fromFilesTextArea->toPlainText();
    if(lsAreaText.lastIndexOf(",") != giCurrentPos){
        giCurrentPos = lsAreaText.lastIndexOf(",");
        QStringList files = lsAreaText.split(",");

        files.removeAt(files.length() - 1);

        for(index = 0; index < files.length(); index++){
            counted = countAllFiles(files.at(index).trimmed());
            if(counted == 0) notFolderCount ++;
            fileCount += counted;
        }

        giTotalFolders -= index;
        fileCount -= giTotalFolders;
        giFileCounter = fileCount;
        giTotalFolders += notFolderCount;
        ui->fileCountLabel->setText(giFileCounter > 0 ? QString::number(giFileCounter):QString::number(0));
        ui->folderCountLabel->setText(giTotalFolders > 0 ? QString::number(giTotalFolders):QString::number(0));

        if(ui->toFilesTextField->text() != NULL && !(ui->toFilesTextField->text() == "") && giFileCounter > 0){
            ui->backupButton->setEnabled(true);
        }else{
            ui->backupButton->setEnabled(false);
        }
    }
}

void BUMain::on_logViewerButton_clicked()
{
    gobLogViewer->setVisible(true);
}

void BUMain::main_slot_showMessage(QString message)
{
    QMessageBox::critical(this,"Error",message,QMessageBox::Ok,QMessageBox::Ok);
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
}
