#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QThread>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QDirIterator>

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = 0);
    void setFileCounter(int value);
    bool createDirectory(QString path);

signals:
    void worker_Signal_updateProgressBar(int value);
    void worker_signal_keepCopying();
    void worker_signal_logInfo(QString info);
    void worker_signal_errorOnCopy();
    void worker_signal_statusInfo(QString info);
    void worker_signal_showMessage(QString message, int messageType);
    void worker_signal_sendDirAndFileList(QStringList *, QStringList *);
    void worker_signal_setTotalFilesAndFolders(int totalFiles, int totalFolders, qint64 totalFileSize);
    void worker_signal_copyNextFile();
    void worker_signal_scanReady();
    void worker_signal_workerDone();

public slots:
    void worker_slot_setStopFlag(int value);
    void worker_slot_createDirs(QString sourceFileOrFolder, QString destinationFolder, int giKeep);
    void worker_slot_readyToStartCopy();
    void worker_slot_copyFile(QString srcFilePath, QString tgtFilePath);
    void worker_slot_scanFolders(QString aobFolderPath);
    void worker_slot_scanNextPath();
    void worker_slot_renameEnable(bool value);

private:
    bool copyDirsRecursively(QString srcFilePath, QString tgtFilePath);
    void countAllFiles(QString path);
    void emitCountersSignals();

    int giCurrentFileIndex;         //Current file index for progressbar and logs.
    int giStopDirCopy;              //Stop copy flag
    int giFoldersCounter;           //Total number of folders counted
    int giFileCounter;              //Total number of files counted
    int giTotalFilesAccumulator;    //Accumulator for total files label
    int emitFlag;                   //Flag for file counting control
    bool gbIsRenameEnabled;         //Flag for enable automatic file renaming
    qint64 giTotalFilesSize;        //Accumulator for total size label
    QDirIterator *gobIterator;      //Global directory iterator for file counting
    QFile *gobFile;                 //Current file under scanning
    QStringList *gobDirList;        //Source directories list
    QStringList *gobFilesList;      //Source files list

};

#endif // WORKER_H
