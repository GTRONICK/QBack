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

signals:
    void worker_Signal_updateProgressBar(int value);
    void worker_signal_keepCopying();
    void worker_signal_logInfo(QString info);
    void worker_signal_statusInfo(QString info);
    void worker_signal_showMessage(QString message);
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

private:
    bool copyRecursively(QString srcFilePath, QString tgtFilePath);
    void countAllFiles(QString path);
    void emitCountersSignals();

    int giCurrentFileIndex;        //Current file index for progressbar and logs.
    int giStopDirCopy;             //Stop copy flag
    int giFoldersCounter;          //Total number of folders counted
    int giFileCounter;             //Total number of files counted
    int giTotalFilesAccumulator;   //Accumulator for total files label
    qint64 giTotalFilesSize;       //Accumulator for total size label
    QStringList gobFileList;       //Current dir entryList
    QDir gobDir;                   //Current dir under scanning
    QDirIterator *gobIterator;      //Global directory iterator for file counting
    QFile *gobFile;
    QStringList *gobDirList;
    QStringList *gobFilesList;
    int emitFlag;
};

#endif // WORKER_H
