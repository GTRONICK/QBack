#ifndef BUMAIN_H
#define BUMAIN_H

#define TOOLTIP_DURATION 2000
#include <QMainWindow>
#include <QThread>
#include <QFileDialog>
#include <QStringList>
#include "worker.h"
#include "logviewer.h"
#include "searchdialog.h"

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
    void main_signal_scanFolders(QStringList folderPaths);
    void main_signal_setTextEdit(QPlainTextEdit *textEdit);

private slots:
    void on_backupButton_clicked();
    void on_originButton_clicked();
    void on_targetButton_clicked();
    void on_toFilesTextField_textChanged();
    void on_openTargetButton_clicked();
    void on_fromFilesTextArea_textChanged();
    void on_logViewerButton_clicked();
    void on_actionQuit_triggered();
    void on_actionAbout_triggered();
    void on_actionOpen_session_triggered();
    void on_actionLoad_theme_triggered();
    void on_actionSave_session_triggered();
    void on_actionFind_in_sources_triggered();
    void main_slot_showMessage(QString message);
    void main_slot_keepCopying();
    void main_slot_setStatus(QString status);
    void main_slot_receiveDirAndFileList(QStringList *dirs, QStringList *files);
    void main_slot_copyNextFile();
    void main_slot_resetCursor();
    void main_slot_setTotalFilesAndFolders(int aiFileCounter, int aiFolderCounter, qint64 aiTotalFilesSize);
    void main_slot_workerDone();
    void main_slot_getTextEdit();

private:
    void initThreadSetup();
    int  countAllFiles(QString path);
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
    SearchDialog *gobSearchDialog;

    int giKeep;                     //Stop copy flag
    int giCurrentPos;               //Current carret position in the fromFilesTextArea
    int giFileCounter;              //Total files counter
    qint64 giTotalFilesSize;          //Total files size to be copied
    int giProgress;                 //Copy progress counter
    int giTotalFolders;             //Total folder counter
    int validatorFlag;              //Backup button disable flag
    int giCopyFileIndex;            //Index for the current file being copied
    bool gbBackcupButtonPressed;    //Backup button control flag (true if pressed)
};

#endif // BUMAIN_H
