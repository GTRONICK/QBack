#ifndef BUMAIN_H
#define BUMAIN_H

#define TOOLTIP_DURATION 2000

#include <QMainWindow>
#include <QEvent>
#include <QThread>
#include <QFileDialog>
#include <QStringList>
#include "worker.h"
#include "logviewer.h"

namespace Ui {
class BUMain;
}

class BUMain : public QMainWindow
{
    Q_OBJECT

public:
    explicit BUMain(QWidget *parent = 0);
    bool eventFilter(QObject *obj, QEvent *event);
    ~BUMain();

signals:
    void main_signal_copyFile(QString file,QString path);
    void main_startCopyFiles(QStringList files,QString path);
    void main_signal_saveLogToFile(bool value);

private slots:
    void on_backupButton_clicked();
    void on_originButton_clicked();
    void on_targetButton_clicked();
    void on_toFilesTextField_textChanged();
    void on_helpButton_clicked();
    void on_openTargetButton_clicked();
    void on_fromFilesTextArea_textChanged();
    void on_logViewerButton_clicked();

    void main_slot_showMessage(QString message);
    void main_slot_keepCopying();
    void main_slot_setStatus(QString status);
    void main_slot_fileSize(qint64 size);
    void main_slot_setCurrentFileProgressStatus(qint64 progress);

    void on_cancelButton_clicked();

private:
    void initThreadSetup();
    int countAllFiles(QString path);
    void installEventFilters();
    void uninstallEventFilters();

    Ui::BUMain *ui;
    QThread *thread;
    Worker *worker;
    QFileDialog *dialog;
    QString targetFolder;
    QStringList gobPaths;
    LogViewer *gobLogViewer;
    int giCurrentPos;
    int giPreviousPos;
    int giFileCounter;
    int giProgress;
    int giTotalFiles;
    int giTotalFolders;
    int validatorFlag;
};

#endif // BUMAIN_H
