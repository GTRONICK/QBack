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
        if(!copyRecursively(sourceFileOrFolder,destinationFolder)){
            emit(worker_signal_logInfo("Folder can't be copied!"));
            emit(worker_signal_showMessage("Folder cannot be copied. It may already exist or you don't have enought permissions \nOpen the target and verify if it already exist"));
            this->worker_slot_setStopFlag(1);
        }else{
            emit(worker_signal_keepCopying());
        }
    }
}

void Worker::worker_slot_readyToStartCopy()
{
    // qDebug() << "worker: readyToStartCopy SIGNAL received";
    // qDebug() << "worker: Emmiting sendDirAndFileList SIGNAL";
    emit(worker_signal_sendDirAndFileList(gobDirList,gobFilesList));
}

void Worker::worker_slot_copyFile(QString srcFilePath, QString tgtFilePath)
{
    // qDebug() << "worker: copyFile SIGNAL received with " << srcFilePath << ", " << tgtFilePath;
    QString fileName;
    QString extension = "";
    if(!QFileInfo(srcFilePath).completeSuffix().isEmpty()){
        extension = "."+QFileInfo(srcFilePath).completeSuffix();
    }

    fileName = QFileInfo(srcFilePath).baseName() + extension;

    if (QFile::exists(tgtFilePath+"/"+fileName)){
        QFile::remove(tgtFilePath+"/"+fileName);
    }

    gobFile = new QFile(srcFilePath);

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
                emit(worker_signal_logInfo(QDateTime::currentDateTime().toString() + " ERROR! File: " + fileName + " does not exist."));
            }else if(!QDir(tgtFilePath).exists()){
                emit(worker_signal_logInfo(QDateTime::currentDateTime().toString() + " ERROR! Folder: " + tgtFilePath + " does not exist."));
            }else{
                emit(worker_signal_logInfo(QDateTime::currentDateTime().toString() + " ERROR! File: " + fileName + " could not be copied. The file might be locked by another process."));
            }
        }

        // qDebug() << "worker: Emmiting SIGNAL copyNextFile";
        emit(worker_signal_copyNextFile());
    }
}

void Worker::worker_slot_scanFolders(QString aobFolderPath)
{
    // qDebug() << "Worker: scanFolders SIGNAL received";
    emit(worker_signal_statusInfo("Counting files, please wait..."));
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
}

void Worker::countAllFiles(QString path)
{

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
            // qDebug() << "worker: Emitting scanReady SIGNAL";
            emit(worker_signal_scanReady());
        }

    }else{
        giFileCounter ++;
        giTotalFilesSize += (lobFileInfo.size());
        emitCountersSignals();
        emit(worker_signal_workerDone());
    }
}

void Worker::worker_slot_scanNextPath()
{
    // qDebug() << "worker: scanNextPath SIGNAL received";
    if(gobIterator->hasNext()){
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
        emit(worker_signal_workerDone());
    }
}

bool Worker::copyRecursively(QString srcFilePath, QString tgtFilePath)
{   
    QFileInfo source(srcFilePath);
    QDir::Filters lobFilters = QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System | QDir::NoSymLinks;

    if(source.isDir()){
        QDir leafDir(srcFilePath);
        leafDir.setFilter(lobFilters);
        if(source.baseName().isEmpty()){
            if(!leafDir.mkdir(tgtFilePath + QLatin1Char('/') + "." + source.completeSuffix())){
                // qDebug() << "Folder " + tgtFilePath + QLatin1Char('/') + "." + source.completeSuffix() + " cannot be copied!";
                return false;
            }

            QDir sourceDir(srcFilePath);
            QStringList fileNames = sourceDir.entryList(lobFilters);
            for(int index = 0; index < fileNames.length(); index ++){
                copyRecursively(sourceDir.absolutePath() + "/" + fileNames.at(index),tgtFilePath + QLatin1Char('/') + "." + source.completeSuffix());
            }

        }else{
            if(!leafDir.mkdir(tgtFilePath + QLatin1Char('/') + source.baseName())){
                // qDebug() << "Folder " << leafDir << " cannot be copied!";
                return false;
            }else{

                QDir sourceDir(srcFilePath);
                QStringList fileNames = sourceDir.entryList(lobFilters);
                for(int index = 0; index < fileNames.length(); index ++){
                    copyRecursively(sourceDir.absolutePath() + "/" + fileNames.at(index),tgtFilePath + QLatin1Char('/') + source.baseName());
                }
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

void Worker::emitCountersSignals()
{
    emit(worker_signal_statusInfo("Ready."));
    emit(worker_signal_setTotalFilesAndFolders(giFileCounter,giFoldersCounter,giTotalFilesSize / 1000000));
}
