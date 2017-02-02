#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
#include <QPlainTextEdit>

namespace Ui {
class SearchDialog;
}

class SearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchDialog(QWidget *parent = 0);
    void setGlobalText(QPlainTextEdit *textEdit);
    ~SearchDialog();

signals:
    void search_signal_resetCursor();

private slots:
    void on_searchDialog_searchButton_clicked();


    void on_searchDialog_replaceButton_clicked();

    void on_searchDialog_replaceAllButton_clicked();

private:
    Ui::SearchDialog *ui;
    int giLine;
    int giLogCursorPos;
    int giOcurrencesFound;
    QString gsFoundText;
    QPlainTextEdit *gobTextEdit;
};

#endif // SEARCHDIALOG_H
