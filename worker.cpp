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

/**
 * @brief Worker::ResetFileCounter
 * Resets the file counter variable, and the paths lists.
 */
void Worker::ResetFileCounter()
{
    //qDebug() << "Begin ResetFileCounter";
    giCurrentFileIndex = 0;
    gobDirList->clear();
    gobFilesList->clear();
    //qDebug() << "End ResetFileCounter";
}

/**
 * @brief Worker::worker_slot_setStopFlag
 * Sets the flag to stop the current copy.
 * @param value 1: Stop, Other value: Continue.
 */
void Worker::worker_slot_setStopFlag(int value)
{
    //qDebug() << "worker_slot_setStopFlag, value = " << QString::number(value);
    giStopDirCopy = value;
    if(value == 1){
        this->ResetFileCounter();
    }
    //qDebug() << "End worker_slot_setStopFlag";
}

/**
 * @brief Worker::worker_slot_createDirs
 * Validates the creation of the directory passed as argument.
 * @param asSourceFileOrFolder Source file, or folder to be copied.
 * @param asDestinationFolder Folder where the file, or folder, will be copied to.
 * @param aiKeep Flag to control the copy of the next path.
 */
void Worker::worker_slot_createDirs(QString asSourceFileOrFolder,QString asDestinationFolder,int aiKeep)
{
    //qDebug() << "Begin worker_slot_createDirs, asSourceFileOrFolder=" << asSourceFileOrFolder << " asDestinationFolder=" << asDestinationFolder;
    if(aiKeep == 0){
        if(!copyDirsRecursively(asSourceFileOrFolder,asDestinationFolder)){
            emit worker_signal_logInfo("Folder can't be copied!, it may already exist in the destintaion target");
            emit worker_signal_showMessage("Folder cannot be copied. It may already exist or you don't have enought permissions \nOpen the target and verify if it already exist", QMessageBox::Critical);
            this->worker_slot_setStopFlag(1);
        }else{
            emit(worker_signal_keepCopying());
        }
    }
    //qDebug() << "End worker_slot_createDirs";
}

/**
 * @brief Worker::worker_slot_readyToStartCopy
 * Send the updated files and folders arrays to start the copy.
 */
void Worker::worker_slot_readyToStartCopy()
{
    //qDebug() << "Begin worker_slot_readyToStartCopy";
    emit worker_signal_sendDirAndFileList(gobDirList,gobFilesList);
    //qDebug() << "End worker_slot_readyToStartCopy";
}

/**
 * @brief Worker::worker_slot_copyFile
 * Copies a file to the especified folder.
 * @param asSourceFilePath
 * @param asTargetFilePath
 */
void Worker::worker_slot_copyFile(QString asSourceFilePath, QString asTargetFilePath)
{
    //qDebug() << "Begin worker_slot_copyFile, asSourceFilePath=" << asSourceFilePath << " asTargetFilePath=" << asTargetFilePath;
    QString fileName;
    QString extension = "";
    QStringList matches;
    QString lsSource = asSourceFilePath;
    QString lsTarget = asTargetFilePath;

    if(!QFileInfo(lsSource).completeSuffix().isEmpty()){
        extension = "."+QFileInfo(lsSource).completeSuffix();
    }

    fileName = QFileInfo(lsSource).baseName() + extension;

    gobFile = new QFile(lsSource);

    if(gbIsRenameEnabled){

        matches = gobFilesList->filter(QRegExp(fileName));

        if(matches.length() > 1){
            QString lsDirName = QFileInfo(lsTarget).dir().absolutePath()+ "/" + QFileInfo(lsSource).dir().absolutePath().replace(":/","/");
            QDir lobDir;
            lobDir.mkpath(lsDirName);
            fileName = QFileInfo(lsSource).baseName() + extension;
            lsTarget = lsDirName;
        }
    }

    if (QFile::exists(lsTarget+"/"+fileName)){
        QFile::remove(lsTarget+"/"+fileName);
    }

    emit(worker_signal_statusInfo("Copying file #" + QString::number(giCurrentFileIndex + 1) + " : " + fileName));

    QApplication::processEvents();

    if(giStopDirCopy == 0){
        if(gobFile->copy(lsTarget+"/"+fileName)){
            QApplication::processEvents();
            giCurrentFileIndex ++;
            emit(worker_signal_logInfo(QDateTime::currentDateTime().toString() + ":  " + "#" + QString::number(giCurrentFileIndex) + "  " + lsSource + "   copied to:   " + lsTarget));
            emit(worker_Signal_updateProgressBar(giCurrentFileIndex));
        }else{
            if(!QFile(lsSource).exists()){
                emit(worker_signal_logInfo(QDateTime::currentDateTime().toString() + " ERROR! File: " + lsSource + " does not exist."));
            }else if(!QDir(lsTarget).exists()){
                emit(worker_signal_logInfo(QDateTime::currentDateTime().toString() + " ERROR! Folder: " + lsSource + " does not exist."));
            }else{
                emit(worker_signal_logInfo(QDateTime::currentDateTime().toString() + " ERROR! File: " + lsSource + " could not be copied. The file might be locked by another process."));
            }
            emit worker_signal_errorOnCopy();
        }
        emit(worker_signal_copyNextFile());
    }
    //qDebug() << "End worker_slot_copyFile";
}

/**
 * @brief Worker::worker_slot_scanFolders
 * Prepares the worker for count the files inside the path.
 * @param aobFolderPath
 */
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

/**
 * @brief Worker::countAllFiles
 * Counts all the files inside a directory.
 * @param asPathToScan
 */
void Worker::countAllFiles(QString asPathToScan)
{
    //qDebug() << "Begin countAllFiles, asPathToScan=" << asPathToScan;
    gobIterator = NULL;
    QFileInfo lobFileInfo(asPathToScan);
    if(lobFileInfo.isDir()){
        gobIterator = new QDirIterator(asPathToScan,QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System | QDir::NoSymLinks,QDirIterator::Subdirectories);
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
    //qDebug() << "End countAllFiles";
}

/**
 * @brief Worker::worker_slot_scanNextPath
 * Slot used to scan the files, in the next path if available.
 */
void Worker::worker_slot_scanNextPath()
{
    //qDebug() << "Begin worker_slot_scanNextPath";
    if(gobIterator!=NULL && gobIterator->hasNext()){
        QFileInfo source(gobIterator->next());
        if(source.isFile()){
            giFileCounter ++;
            giTotalFilesSize += (source.size());
        }else{
            giFoldersCounter ++;
        }

        if(giFileCounter % 50 == 0) emit(worker_signal_setTotalFilesAndFolders(giFileCounter,giFoldersCounter,giTotalFilesSize / 1000000));
        emit(worker_signal_scanReady());
    }else if(emitFlag == 0){
        emitFlag = 1;
        emitCountersSignals();
        emit worker_signal_workerDone();
    }
    //qDebug() << "End worker_slot_scanNextPath";
}

/**
 * @brief Worker::worker_slot_renameEnable
 * Slot used to control the autorename feature.
 * @param abValue
 */
void Worker::worker_slot_renameEnable(bool abValue)
{
    //qDebug() << "Begin worker_slot_renameEnable";
    gbIsRenameEnabled = abValue;
    //qDebug() << "End worker_slot_renameEnable";
}

/**
 * @brief Worker::copyDirsRecursively
 * Copies a directory in a recursive manner.
 * @param asSourceFilePath
 * @param asTargetFilePath
 * @return
 */
bool Worker::copyDirsRecursively(QString asSourceFilePath, QString asTargetFilePath)
{   
    //qDebug() << "Begin copyDirsRecursively";
    QString lsTargetDir;
    QFileInfo source(asSourceFilePath);
    QDir::Filters lobFilters = QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System | QDir::NoSymLinks;

    if(!createDirectory(asTargetFilePath)){
        emit worker_signal_logInfo("Folder: " + asTargetFilePath + " cannot be created. You have no write permissions on the target.");
        emit worker_signal_showMessage("Error creating target folder. See the log for details.", QMessageBox::Critical);
        //qDebug() << "End copyDirsRecursively, false";
        return false;
    }

    if(source.isDir()){

        QDir sourceDir(asSourceFilePath);
        QStringList fileNames = sourceDir.entryList(lobFilters);

        sourceDir.setFilter(lobFilters);

        if(source.baseName().isEmpty()){
            lsTargetDir = asTargetFilePath + QLatin1Char('/') + "." + source.completeSuffix();
        }else{
            lsTargetDir = asTargetFilePath + QLatin1Char('/') + source.fileName();
        }

        if(!sourceDir.mkdir(lsTargetDir)){
            emit worker_signal_logInfo("Folder: " + lsTargetDir + " cannot be created. It may already exist");
        }

        for(int index = 0; index < fileNames.length(); index ++){
            copyDirsRecursively(sourceDir.absolutePath() + "/" + fileNames.at(index),lsTargetDir);
        }

    }else{

        QApplication::processEvents();
        if(giStopDirCopy == 0){
            gobFilesList->append(asSourceFilePath);
            gobDirList->append(asTargetFilePath+"/");
        }
    }

    //qDebug() << "End copyDirsRecursively, true";
    return true;
}

/**
 * @brief Worker::createDirectory
 * Creates the specified directory.
 * @param asPath
 * @return
 */
bool Worker::createDirectory(QString asPath)
{
    //qDebug() << "Begin createDirectory, asPath=" << asPath;
    QDir targetDir(asPath);
    //qDebug() << "End createDirectory";
    return targetDir.mkpath(asPath);
}

/**
 * @brief Worker::emitCountersSignals
 * Emits the signals to update the files and folders counters.
 */
void Worker::emitCountersSignals()
{
    //qDebug() << "Begin emitCountersSignals";
    emit(worker_signal_statusInfo("Ready."));
    emit(worker_signal_setTotalFilesAndFolders(giFileCounter,giFoldersCounter,giTotalFilesSize / 1000000));
    //qDebug() << "End emitCountersSignals";
}
