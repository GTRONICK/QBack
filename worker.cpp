#include "worker.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QStringList>
#include <QCoreApplication>
#include <QDateTime>
#include <QMessageBox>
#include <QApplication>
#include <QDirIterator>

Worker::Worker(QObject *parent) :
    QObject(parent)
{
    giCurrentFileIndex = 0;
    giStopDirCopy = 0;
    giFileCounter = 0;
    giFoldersCounter = 0;
    giTotalFilesSize = 0;
    giTotalFilesAccumulator = 0;
    gbIsRenameEnabled = true;
    gobDirList = new QStringList;
    gobFilesList = new QStringList;
}

void Worker::setFileCounter(int value)
{
    giCurrentFileIndex = value;
    gobDirList->clear();
    gobFilesList->clear();
}

void Worker::worker_slot_setStopFlag(int value)
{
    // qDebug() << "worker: setStopFlag SIGNAL received with value: " << value;
    giStopDirCopy = value;
    if(value == 1){
        // qDebug() << "worker: Clearing lists";
        gobDirList->clear();
        gobFilesList->clear();
        giCurrentFileIndex = 0;
    }
}

void Worker::worker_slot_createDirs(QString sourceFileOrFolder,QString destinationFolder,int giKeep)
{
    if(giKeep == 0){
        if(!copyDirsRecursively(sourceFileOrFolder,destinationFolder)){
            emit worker_signal_logInfo("Folder can't be copied!, it may already exist in the destintaion target");
            emit worker_signal_showMessage("Folder cannot be copied. It may already exist or you don't have enought permissions \nOpen the target and verify if it already exist", QMessageBox::Critical);
            this->worker_slot_setStopFlag(1);
        }else{
            emit(worker_signal_keepCopying());
        }
    }
}

void Worker::worker_slot_readyToStartCopy()
{
    emit(worker_signal_sendDirAndFileList(gobDirList,gobFilesList));
}

void Worker::worker_slot_copyFile(QString srcFilePath, QString tgtFilePath)
{
    // qDebug() << "worker: copyFile SIGNAL received with " << srcFilePath << ", " << tgtFilePath;
    QString fileName;
    QString extension = "";
    QStringList matches;

    if(!QFileInfo(srcFilePath).completeSuffix().isEmpty()){
        extension = "."+QFileInfo(srcFilePath).completeSuffix();
    }

    fileName = QFileInfo(srcFilePath).baseName() + extension;

    gobFile = new QFile(srcFilePath);

    if(gbIsRenameEnabled){

        matches = gobFilesList->filter(QRegExp(fileName));

        if(matches.length() > 1){
            fileName = QFileInfo(srcFilePath).baseName() +
                    "_" + QFileInfo(srcFilePath).dir().absolutePath().replace(QRegExp("[\\/:]"),"_") +
                    extension;
        }
    }

    if (QFile::exists(tgtFilePath+"/"+fileName)){
        QFile::remove(tgtFilePath+"/"+fileName);
    }

    emit(worker_signal_statusInfo("Copying file #" + QString::number(giCurrentFileIndex + 1) + " : " + fileName));

    QApplication::processEvents();

    if(giStopDirCopy == 0){
        if(gobFile->copy(tgtFilePath+"/"+fileName)){
            QApplication::processEvents();
            giCurrentFileIndex ++;
            emit(worker_signal_logInfo(QDateTime::currentDateTime().toString() + ":  " + "#" + QString::number(giCurrentFileIndex) + "  " + srcFilePath + "   copied to:   " + tgtFilePath));
            emit(worker_Signal_updateProgressBar(giCurrentFileIndex));
        }else{
            if(!QFile(srcFilePath).exists()){
                emit(worker_signal_logInfo(QDateTime::currentDateTime().toString() + " ERROR! File: " + srcFilePath + " does not exist."));
            }else if(!QDir(tgtFilePath).exists()){
                emit(worker_signal_logInfo(QDateTime::currentDateTime().toString() + " ERROR! Folder: " + tgtFilePath + " does not exist."));
            }else{
                emit(worker_signal_logInfo(QDateTime::currentDateTime().toString() + " ERROR! File: " + srcFilePath + " could not be copied. The file might be locked by another process."));
            }
            emit worker_signal_errorOnCopy();
        }
        // qDebug() << "worker: Emmiting SIGNAL copyNextFile";
        emit(worker_signal_copyNextFile());

    }
}

void Worker::worker_slot_scanFolders(QString aobFolderPath)
{
    //qDebug() << "Begin worker_slot_scanFolders " << "aobFolderPath: " << aobFolderPath;
    emit worker_signal_statusInfo("Counting files, please wait...");
    giFoldersCounter = 0;
    giFileCounter = 0;
    giTotalFilesSize = 0;
    giTotalFilesAccumulator = 0;
    emitFlag = 0;
    gobDirList->clear();
    gobFilesList->clear();
    if(aobFolderPath.contains(">")){
        countAllFiles(aobFolderPath.split(">").at(0).trimmed());
    }
    else {
        countAllFiles(aobFolderPath.trimmed());
    }
    //qDebug() << "End worker_slot_scanFolders";
}

void Worker::countAllFiles(QString path)
{
    gobIterator = NULL;
    QFileInfo lobFileInfo(path);
    if(lobFileInfo.isDir()){
        gobIterator = new QDirIterator(path,QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System | QDir::NoSymLinks,QDirIterator::Subdirectories);
        if(gobIterator->hasNext()){
            QFileInfo source(gobIterator->next());
            if(source.isFile()){
                giFileCounter ++;
                giTotalFilesSize += (source.size());
            }else{
                giFoldersCounter ++;
            }

            if(giFileCounter % 50 == 0) emit(worker_signal_setTotalFilesAndFolders(giFileCounter,giFoldersCounter,giTotalFilesSize / 1000000));
            //qDebug() << "worker: Emitting scanReady SIGNAL";
            emit worker_signal_scanReady();
        }else{
            emitCountersSignals();
            emit worker_signal_workerDone();
        }

    }else{
        giFileCounter ++;
        giTotalFilesSize += (lobFileInfo.size());
        emitCountersSignals();
        emit worker_signal_workerDone();
    }
}

void Worker::worker_slot_scanNextPath()
{
    // qDebug() << "worker: scanNextPath SIGNAL received";
    if(gobIterator!=NULL && gobIterator->hasNext()){
        QFileInfo source(gobIterator->next());
        if(source.isFile()){
            giFileCounter ++;
            giTotalFilesSize += (source.size());
        }else{
            giFoldersCounter ++;
        }

        if(giFileCounter % 50 == 0) emit(worker_signal_setTotalFilesAndFolders(giFileCounter,giFoldersCounter,giTotalFilesSize / 1000000));
        // qDebug() << "worker: emitting scanReady SIGNAL";
        emit(worker_signal_scanReady());
    }else if(emitFlag == 0){
        emitFlag = 1;
        // qDebug() << "worker: Emitting counters SIGNALS from worker_slot_scanNextPath";
        emitCountersSignals();
        emit worker_signal_workerDone();
    }
}

void Worker::worker_slot_renameEnable(bool value)
{
    gbIsRenameEnabled = value;
}

/**
  Copies directories recursively.
  @param srcFilePath Source folder.
  @param tgtFilePath Target folder.
  @return True, if the copy was successful, false otherwise.
*/
bool Worker::copyDirsRecursively(QString srcFilePath, QString tgtFilePath)
{   
    QString lsTargetDir;
    bool unixHidden;
    QFileInfo source(srcFilePath);
    QDir::Filters lobFilters = QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System | QDir::NoSymLinks;

    if(!createDirectory(tgtFilePath)){
        emit worker_signal_logInfo("Folder: " + tgtFilePath + " cannot be created. You have no write permissions on the target.");
        emit worker_signal_showMessage("Error creating target folder. See the log for details.", QMessageBox::Critical);
        return false;
    }

    if(source.isDir()){

        QDir sourceDir(srcFilePath);
        QStringList fileNames = sourceDir.entryList(lobFilters);

        sourceDir.setFilter(lobFilters);

        if(source.baseName().isEmpty()){
            lsTargetDir = tgtFilePath + QLatin1Char('/') + "." + source.completeSuffix();
            unixHidden = true;
        }else{
            lsTargetDir = tgtFilePath + QLatin1Char('/') + source.baseName();
            unixHidden = false;
        }

        if(!sourceDir.mkdir(lsTargetDir)){
            emit worker_signal_logInfo("Folder: " + lsTargetDir + " cannot be created. It may already exist");
        }

        if(unixHidden) {
            for(int index = 0; index < fileNames.length(); index ++){
                copyDirsRecursively(sourceDir.absolutePath() + "/" + fileNames.at(index),tgtFilePath + QLatin1Char('/') + "." + source.completeSuffix());
            }
        }else {
            for(int index = 0; index < fileNames.length(); index ++){
                copyDirsRecursively(sourceDir.absolutePath() + "/" + fileNames.at(index),tgtFilePath + QLatin1Char('/') + source.baseName());
            }
        }

    }else{

        QApplication::processEvents();
        if(giStopDirCopy == 0){
            gobFilesList->append(srcFilePath);
            gobDirList->append(tgtFilePath+"/");
        }
    }

    return true;
}

bool Worker::createDirectory(QString path)
{
    QDir targetDir(path);
    if(!targetDir.mkpath(path)){
        return false;
    }
    return true;
}

void Worker::emitCountersSignals()
{
    //qDebug() << "Begin emitCountersSignals";
    emit(worker_signal_statusInfo("Ready."));
    emit(worker_signal_setTotalFilesAndFolders(giFileCounter,giFoldersCounter,giTotalFilesSize / 1000000));
    //qDebug() << "End emitCountersSignals";
}
