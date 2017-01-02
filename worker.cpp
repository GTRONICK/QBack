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
    fileList = new QStringList;
    dirList = new QStringList;
}

void Worker::setFileCounter(int value)
{
    giTotalFiles = value;
}

void Worker::worker_slot_setStopFlag(int value)
{
    giStopDirCopy = value;
}

void Worker::worker_slot_createDirStructure(QString sourceFileOrFolder,QString destinationFolder,int giKeep)
{
    fileList->clear();
    dirList->clear();
    emit(worker_signal_statusInfo("Creating directory structure..."));
    if(giKeep == 0){
        if(!copyRecursively(sourceFileOrFolder,destinationFolder)){
            emit(worker_signal_logInfo("Folder can't be copied!"));
            emit(worker_signal_showMessage("Folder cannot be copied. It may already exist or you don't have enought permissions \nOpen the target and verify if it already exist"));
        }

        emit(worker_signal_sendFileList(fileList,dirList));
        emit(worker_signal_keepCopying());
    }

//    for(int i = 0; i < dirList->length(); i++) qDebug() << dirList->at(i);
}

void Worker::worker_slot_copyFile(QString srcFilePath, QString tgtFilePath, int giKeep)
{
    if(giKeep == 0){
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
            }else{
                emit(worker_signal_logInfo(QDateTime::currentDateTime().toString() + " >> ERROR! File: " + fileName + " << has not been copied!"));
            }
        }

        emit(worker_signal_copyReady());
    }
}

void Worker::printToConsole(QString text)
{
    qDebug() << text;
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
            dirList->append(tgtFilePath + QLatin1Char('/') + source.baseName());
            copyRecursively(sourceDir.absolutePath() + "/" + fileNames.at(index),tgtFilePath + QLatin1Char('/') + source.baseName());
        }
    }
    else{
        fileList->append(srcFilePath);
    }

    return true;

}
