#include "worker.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QStringList>
#include <QCoreApplication>
#include <QDateTime>
#include <QMessageBox>

Worker::Worker(QObject *parent) :
    QObject(parent)
{
    giTotalFiles = 0;
}

void Worker::setFileCounter(int value)
{
    giTotalFiles = value;
}

void Worker::worker_Slot_copyFile(QString sourceFileOrFolder,QString destinationFolder,int giKeep)
{
    if(giKeep == 0){
        if(!copyRecursively(sourceFileOrFolder,destinationFolder)){
            emit(worker_signal_logInfo("Folder can't be copied!"));
            emit(worker_signal_showMessage("Folder cannot be copied. It may already exist or you don't have enought permissions \nOpen the target and verify if it already exist"));
        }

        emit(worker_signal_keepCopying());
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
            copyRecursively(sourceDir.absolutePath() + "/" + fileNames.at(index),tgtFilePath + QLatin1Char('/') + source.baseName());
        }

    }else{
        QString fileName;

        fileName = QFileInfo(srcFilePath).baseName()+"."+QFileInfo(srcFilePath).completeSuffix();

        if (QFile::exists(tgtFilePath+"/"+fileName)){

            QFile::remove(tgtFilePath+"/"+fileName);
        }

        gobFile = new QFile(srcFilePath);

        emit(worker_signal_statusInfo("Copying file #" + QString::number(giTotalFiles + 1) + " : " + fileName));

        if(gobFile->copy(tgtFilePath+"/"+fileName)){
            giTotalFiles ++;
            emit(worker_signal_logInfo(QDateTime::currentDateTime().toString() + " >> File: " + fileName + " << copied to: " + tgtFilePath));
            emit(worker_Signal_updateProgressBar(giTotalFiles));
        }else{
            emit(worker_signal_logInfo(QDateTime::currentDateTime().toString() + " >> ERROR! File: " + fileName + " << has not been copied!"));
        }

    }

    return true;
}
