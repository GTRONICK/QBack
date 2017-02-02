#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QThread>
#include <QStringList>
#include <QFile>

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
    void worker_signal_setTotalFilesAndFolders(int totalFiles, int totalFolders);
    void worker_signal_copyNextFile();
    void worker_signal_workerDone();

public slots:
    void worker_slot_setStopFlag(int value);
    void worker_slot_createDirs(QString sourceFileOrFolder, QString destinationFolder, int giKeep);
    void worker_slot_readyToStartCopy();
    void worker_slot_copyFile(QString srcFilePath, QString tgtFilePath);
    void worker_slot_scanFolders(QStringList aobFolderPaths);

private:
    bool copyRecursively(QString srcFilePath, QString tgtFilePath);
    void countAllFiles(QString path);
    int giTotalFiles;
    int giStopDirCopy;
    int giTotalFolders;
    int giFileCounter;

    QFile *gobFile;
    QStringList *gobDirList;
    QStringList *gobFilesList;
};

#endif // WORKER_H
