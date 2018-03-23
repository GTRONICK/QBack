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
    void main_signal_scanFolders(QString folderPath);
    void main_signal_setTextEdit(QPlainTextEdit *textEdit);
    void main_signal_scanNextPath();
    void main_signal_renameEnable(bool value);

private slots:
    void on_backupButton_clicked();
    void on_originButton_clicked();
    void on_targetButton_clicked();
    void on_openTargetButton_clicked();
    void on_logViewerButton_clicked();
    void on_fromFilesTextArea_textChanged();
    void on_toFilesTextField_textChanged();
    void on_actionQuit_triggered();
    void on_actionAbout_triggered();
    void on_actionDefault_theme_triggered();
    void on_actionOpen_session_triggered();
    void on_actionLoad_theme_triggered();
    void on_actionSave_session_triggered();
    void on_actionFind_in_sources_triggered();
    void main_slot_showMessage(QString message, int messageType);
    void main_slot_keepCopying();
    void main_slot_setStatus(QString asStatus);
    void main_slot_receiveDirAndFileList(QStringList *dirs, QStringList *files);
    void main_slot_copyNextFile();
    void main_slot_resetCursor();
    void main_slot_setTotalFilesAndFolders(int aiFileCounter, int aiFolderCounter, qint64 aiTotalFilesSize);
    void main_slot_workerDone();
    void main_slot_getTextEdit();
    void main_slot_scanReady();
    void main_slot_disableFileScan();
    void main_slot_enableFileScan();
    void main_slot_processDropEvent(QDropEvent *event);
    void main_slot_errorOnCopy();
    void on_actionEnable_auto_rename_toggled(bool arg1);
    void on_actionInsert_Target_path_triggered();

private:
    void initThreadSetup();
    int  countAllFiles(QString path);
    void installEventFilters();
    void uninstallEventFilters();
    bool saveSessionToFile(QString asFilePath);
    void loadSessionFile(QString asFilePath);
    void on_cancelButton_clicked();
    void resetCounters();
    void checkBackupButton();
    void resetState();
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    QString removeTrailingSlashes(QString lsString);

    Ui::BUMain *ui;
    QThread *thread;                //Global worker thread.
    Worker *worker;                 //Global worker.
    QFileDialog *dialog;            //File dialog object.
    QString gsTargetFile;           //Target folder path.
    QStringList gobPaths;           //Files paths array.
    LogViewer *gobLogViewer;        //Global log viewer object.
    QStringList *targetDirectories; //Target directories array.
    QStringList *sourceFiles;       //Source fies to be copied.
    SearchDialog *gobSearchDialog;  //Global search and replace dialog object.

    int giKeep;                     //Stop copy flag
    int giCurrentPos;               //Current carret position in the fromFilesTextArea for the ',' character
    int giCurrentNumPos;            //Current carret position in the fromFilesTextArea for the '#' character

    int giTotalFiles;               //Total files counter
    int giTotalFolders;             //Total folder counter
    qint64 giTotalFilesSize;        //Total files size to be copied.

    int giTmpTotalFolders;          //Temporal folder counter
    int giTmpTotalFiles;            //Temporal files counter
    qint64 giTmpTotalFilesSize;     //Temporal size accumulator

    int giProgress;                 //Copy progress counter
    int validatorFlag;              //Backup button disable flag
    int giCopyFileIndex;            //Index for the current file being copied
    int giPathIndex;                //Current path index for the Paths array.
    bool gbBackcupButtonPressed;    //Backup button control flag (true if pressed)
    bool gbCountCancel;             //Flag for interrupt the file counting on text change, into the sources text edit.
    bool gbScanDisabled;            //Flag for on_fromFilesTextArea_textChanged activation control.
    bool gbIsCtrlPressed;
    int giErrorOnCopyFlag;          //Flag to indicate a copy failure.

};

#endif // BUMAIN_H
