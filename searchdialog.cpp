#include "searchdialog.h"
#include "ui_searchdialog.h"
#include <QPlainTextEdit>
#include <QDebug>

SearchDialog::SearchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SearchDialog)
{
    ui->setupUi(this);
    gbReplaceClicked = false;
    gbSearchClicked = false;
    gbReplaceAllClicked = false;
}

SearchDialog::~SearchDialog()
{
    delete ui;
}

void SearchDialog::on_searchDialog_searchButton_clicked()
{
    gbSearchClicked = true;
    emit(search_signal_getTextEditText());
}

void SearchDialog::on_searchDialog_replaceButton_clicked()
{
    gbReplaceClicked = true;
    emit(search_signal_getTextEditText());
}

void SearchDialog::on_searchDialog_replaceAllButton_clicked()
{
    gbReplaceAllClicked = true;
    emit(search_signal_getTextEditText());
}

void SearchDialog::search_slot_setTextEdit(QTextEdit *textEdit)
{
    this->gobTextEdit = textEdit;

    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextEdit::ExtraSelection selection;

    if(gbReplaceAllClicked){
        emit(search_signal_disableFilescan());
        gbReplaceAllClicked = false;

        gobTextEdit->extraSelections().clear();
        gobTextEdit->textCursor().clearSelection();

        if(!ui->searchDialog_replaceLineEdit->text().isEmpty()){

            emit(search_signal_resetCursor());
            gsFoundText = ui->seachDialog_searchLineEdit->text();
            while(gobTextEdit->find(ui->seachDialog_searchLineEdit->text())){
                selection.cursor = gobTextEdit->textCursor();
                extraSelections.append(selection);
            }
            gobTextEdit->setExtraSelections(extraSelections);

            QTextCursor cursor;

            for(int i=0; i < extraSelections.length(); i++){

                cursor = extraSelections.at(i).cursor;
                cursor.insertText(ui->searchDialog_replaceLineEdit->text());

            }
        }

        gsFoundText = "";
        gobTextEdit->extraSelections().clear();
        extraSelections.clear();
        //qDebug() << "search: emitting enableFilescan SIGNAL";
        emit(search_signal_enableFilescan());

    }else if(gbSearchClicked){
        gbSearchClicked = false;

        QColor lineColor = QColor(Qt::darkGray).lighter(160);
        selection.format.setBackground(lineColor);
        selection.format.setForeground(QColor(0,0,0));
        gobTextEdit->extraSelections().clear();

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
            if(gobTextEdit->find(ui->seachDialog_searchLineEdit->text())){
                ui->searchDialog_replaceButton->setEnabled(true);
                giLogCursorPos = 1;
            }
            ui->ocurrencesCounterLabel->setText(QString("%1/%2").arg(giLogCursorPos).arg(giOcurrencesFound));
        }else{
            if(!gobTextEdit->find(ui->seachDialog_searchLineEdit->text())){
                //qDebug() << "searchDialog: Emmitting resetCursor SIGNAL";
                emit(search_signal_resetCursor());
                giLogCursorPos = 0;
                if(gobTextEdit->find(ui->seachDialog_searchLineEdit->text())){
                    giLogCursorPos ++;
                }
            }else{
                ui->searchDialog_replaceButton->setEnabled(true);
                giLogCursorPos ++;
            }
            ui->ocurrencesCounterLabel->setText(QString("%1/%2").arg(giLogCursorPos).arg(giOcurrencesFound));
        }
    }else if(gbReplaceClicked){

        gbReplaceClicked = false;

        if(gsFoundText == ui->seachDialog_searchLineEdit->text()){

            gobTextEdit->textCursor().insertText(ui->searchDialog_replaceLineEdit->text());
        }

        ui->searchDialog_replaceButton->setEnabled(false);

    }
}

void SearchDialog::on_gobSwapTextButton_clicked()
{
    QString lsReplace;
    lsReplace = this->ui->seachDialog_searchLineEdit->text();
    this->ui->seachDialog_searchLineEdit->setText(this->ui->searchDialog_replaceLineEdit->text());
    this->ui->searchDialog_replaceLineEdit->setText(lsReplace);
}
