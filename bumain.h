#ifndef BUMAIN_H
#define BUMAIN_H

#define TOOLTIP_DURATION 2000
#define APP_VERSION "110117"
#include <QMainWindow>
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
    void closeEvent(QCloseEvent *event);
    ~BUMain();

signals:
    void main_signal_createDirs(QString file,QString path,int giKeep);
    void main_startCopyFiles(QStringList files,QString path);
    void main_signal_saveLogToFile(bool value);
    void main_signal_setStopFlag(int value);
    void main_signal_readyToStartCopy();
    void main_signal_copyFile(QString file,QString path);
    void main_signal_logInfo(QString info);

private slots:
    void on_backupButton_clicked();
    void on_originButton_clicked();
    void on_targetButton_clicked();
    void on_toFilesTextField_textChanged();
    void on_openTargetButton_clicked();
    void on_fromFilesTextArea_textChanged();
    void on_logViewerButton_clicked();

    void main_slot_showMessage(QString message);
    void main_slot_keepCopying();
    void main_slot_setStatus(QString status);
    void main_slot_receiveDirAndFileList(QStringList *dirs, QStringList *files);
    void main_slot_copyNextFile();

    void on_actionQuit_triggered();

    void on_actionAbout_triggered();

    void on_actionOpen_session_triggered();

    void on_actionLoad_theme_triggered();

    void on_actionSave_session_triggered();

private:
    void initThreadSetup();
    int countAllFiles(QString path);
    void installEventFilters();
    void uninstallEventFilters();
    bool saveSessionToFile(QString filePath);
    void loadSessionFile(QString asFilePath);
    void on_cancelButton_clicked();
    Ui::BUMain *ui;
    QThread *thread;
    Worker *worker;
    QFileDialog *dialog;
    QString targetFolder;
    QStringList gobPaths;
    LogViewer *gobLogViewer;
    QStringList *targetDirectories;
    QStringList *sourceFiles;
    int giKeep;
    int giCurrentPos;
    int giPreviousPos;
    int giFileCounter;
    int giProgress;
    int giTotalFiles;
    int giTotalFolders;
    int validatorFlag;
    int giCopyFileIndex;
    bool gbBackcupButtonPressed;
};

#endif // BUMAIN_H
