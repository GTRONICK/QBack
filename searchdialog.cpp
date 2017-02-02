#include "searchdialog.h"
#include "ui_searchdialog.h"
#include <QPlainTextEdit>
#include <QDebug>

SearchDialog::SearchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SearchDialog)
{
    ui->setupUi(this);
}

void SearchDialog::setGlobalText(QPlainTextEdit *textEdit)
{
    this->gobTextEdit = textEdit;
}

SearchDialog::~SearchDialog()
{
    delete ui;
}

void SearchDialog::on_searchDialog_searchButton_clicked()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextEdit::ExtraSelection selection;
    QColor lineColor = QColor(Qt::darkGray).lighter(160);
    selection.format.setBackground(lineColor);
    selection.format.setForeground(QColor(0,0,0));

    if(gsFoundText != ui->seachDialog_searchLineEdit->text()){
        giOcurrencesFound = 0;
        giLogCursorPos = 0;
        //qDebug() << "searchDialog: Emmitting resetCursor SIGNAL";
        emit(search_signal_resetCursor());
        gsFoundText = ui->seachDialog_searchLineEdit->text();
        while(gobTextEdit->find(ui->seachDialog_searchLineEdit->text())){
            giOcurrencesFound ++;
            selection.cursor = gobTextEdit->textCursor();
            extraSelections.append(selection);
        }
        gobTextEdit->setExtraSelections(extraSelections);
        //qDebug() << "searchDialog: Emmitting resetCursor SIGNAL";
        emit(search_signal_resetCursor());
        if(gobTextEdit->find(ui->seachDialog_searchLineEdit->text())) giLogCursorPos += 1;
        ui->ocurrencesCounterLabel->setText(QString("%1/%2").arg(giLogCursorPos).arg(giOcurrencesFound));
    }else{
        if(!gobTextEdit->find(ui->seachDialog_searchLineEdit->text())){
            //qDebug() << "searchDialog: Emmitting resetCursor SIGNAL";
            emit(search_signal_resetCursor());
            giLogCursorPos = 0;
            if(gobTextEdit->find(ui->seachDialog_searchLineEdit->text())) giLogCursorPos ++;
        }else{
            giLogCursorPos ++;
        }
        ui->ocurrencesCounterLabel->setText(QString("%1/%2").arg(giLogCursorPos).arg(giOcurrencesFound));
    }
}

void SearchDialog::on_searchDialog_replaceButton_clicked()
{
    if(!ui->searchDialog_replaceLineEdit->text().isEmpty() && gobTextEdit->textCursor().hasSelection()
            && gobTextEdit->textCursor().selectedText().toLower() == ui->seachDialog_searchLineEdit->text().toLower()){
        gobTextEdit->textCursor().insertText(ui->searchDialog_replaceLineEdit->text());
    }
}

void SearchDialog::on_searchDialog_replaceAllButton_clicked()
{
    if(!ui->searchDialog_replaceLineEdit->text().isEmpty() && gobTextEdit->textCursor().hasSelection()){
        QList<QTextEdit::ExtraSelection> extraSelections;
        extraSelections = gobTextEdit->extraSelections();
        QTextCursor cursor;

        for(int i=0; i < extraSelections.length(); i++){

            cursor = extraSelections.at(i).cursor;
            cursor.insertText(ui->searchDialog_replaceLineEdit->text());

        }

        gsFoundText = "";
        gobTextEdit->extraSelections().clear();
    }
}
