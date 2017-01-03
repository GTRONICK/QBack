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

Worker::Worker(QObject *parent) :
    QObject(parent)
{
    giTotalFiles = 0;
    giStopDirCopy = 0;
    gobDirList = new QStringList;
    gobFilesList = new QStringList;

}

void Worker::setFileCounter(int value)
{
    giTotalFiles = value;
}

void Worker::worker_slot_setStopFlag(int value)
{
    // qDebug() << "worker: setStopFlag SIGNAL received with value: " << value;
    giStopDirCopy = value;
    if(value == 1){
        // qDebug() << "worker: Clearing lists";
        gobDirList->clear();
        gobFilesList->clear();
        giTotalFiles = 0;
    }
}

void Worker::worker_slot_createDirs(QString sourceFileOrFolder,QString destinationFolder,int giKeep)
{
    if(giKeep == 0){
        if(!copyRecursively(sourceFileOrFolder,destinationFolder)){
            emit(worker_signal_logInfo("Folder can't be copied!"));
            emit(worker_signal_showMessage("Folder cannot be copied. It may already exist or you don't have enought permissions \nOpen the target and verify if it already exist"));
        }

        emit(worker_signal_keepCopying());
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

    fileName = QFileInfo(srcFilePath).baseName()+"."+QFileInfo(srcFilePath).completeSuffix();

    if (QFile::exists(tgtFilePath+"/"+fileName)){

        QFile::remove(tgtFilePath+"/"+fileName);
    }

    gobFile = new QFile(srcFilePath);

    emit(worker_signal_statusInfo("Copying file #" + QString::number(giTotalFiles + 1) + " : " + fileName));

    QApplication::processEvents();
    if(giStopDirCopy == 0){
        if(gobFile->copy(tgtFilePath+"/"+fileName)){
        QApplication::processEvents();
            giTotalFiles ++;
            emit(worker_signal_logInfo(QDateTime::currentDateTime().toString() + " >> File: " + srcFilePath + " << copied to: " + tgtFilePath));
            emit(worker_Signal_updateProgressBar(giTotalFiles));
        }
        else{
            emit(worker_signal_logInfo(QDateTime::currentDateTime().toString() + " >> ERROR! File: " + fileName + " << has not been copied!"));
        }

        // qDebug() << "worker: Emmiting SIGNAL copyNextFile";
        emit(worker_signal_copyNextFile());
    }
}

bool Worker::copyRecursively(QString srcFilePath, QString tgtFilePath)
{
    QFileInfo source(srcFilePath);
    if(source.isDir()){
        QDir leafDir(srcFilePath);
        leafDir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        if(!leafDir.mkdir(tgtFilePath + QLatin1Char('/') + source.baseName()))
            return false;

        QDir sourceDir(srcFilePath);
        QStringList fileNames = sourceDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        for(int index = 0; index < fileNames.length(); index ++){
            copyRecursively(sourceDir.absolutePath() + "/" + fileNames.at(index),tgtFilePath + QLatin1Char('/') + source.baseName());
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
