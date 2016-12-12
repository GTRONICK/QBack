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

public slots:
    void worker_Slot_copyFile(QString sourceFileOrFolder, QString destinationFolder, int giKeep);
    void printToConsole(QString text);

private:
    bool copyRecursively(QString srcFilePath, QString tgtFilePath);
    int giTotalFiles;
    QFile *gobFile;


};

#endif // WORKER_H
