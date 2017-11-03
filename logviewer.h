#ifndef LOGVIEWER_H
#define LOGVIEWER_H
#define MAX_SIZE 20000000.0

#include <QWidget>

namespace Ui {
class LogViewer;
}

class LogViewer : public QWidget
{
    Q_OBJECT

public:
    explicit LogViewer(QWidget *parent = 0);
    ~LogViewer();

private slots:
    void on_clearButton_clicked();

    void on_okButton_clicked();

    void logger_slot_logInfo(QString info);

    void on_findLineEdit_returnPressed();

private:
    Ui::LogViewer *ui;
    int giLine;
    int giLogCursorPos;
    int giOcurrencesFound;
    QString gsFoundText;
    bool loadLogFile();
};

#endif // LOGVIEWER_H
