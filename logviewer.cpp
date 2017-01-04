#include "logviewer.h"
#include "ui_logviewer.h"
#include <QMessageBox>
#include <QTextStream>
#include <QDesktopServices>
#include <QDebug>

LogViewer::LogViewer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogViewer)
{
    ui->setupUi(this);
    giLine = 0;
    giLogCursorPos = 0;
    giOcurrencesFound = 0;
    gsFoundText = "";
    this->loadLogFile();

    QFile styleFile("style.qss");
    styleFile.open(QFile::ReadOnly);
    QString StyleSheet = QLatin1String(styleFile.readAll());
    this->setStyleSheet(StyleSheet);
}

LogViewer::~LogViewer()
{
    delete ui;
}

void LogViewer::on_clearButton_clicked()
{
    ui->plainTextEdit->clear();
    QFile file("backup.log");
    if(file.exists()){
        file.close();
        if(!file.remove()){
            QMessageBox::critical(this,"Error","Cannot delete log file");
        }
    }
}

void LogViewer::on_okButton_clicked()
{
    this->setVisible(false);
}

void LogViewer::logger_slot_logInfo(QString info)
{
    ui->plainTextEdit->appendPlainText(info);

    QFile file("backup.log");
    if (!file.open(QIODevice::WriteOnly | QFile::Append))
           return;

    QTextStream out(&file);
    out << info << "\r\n\n";
    file.close();

    if(info.contains(">> ERROR!")){
        QMessageBox::critical(this,"Error","Some files could not be copied,\nSee the log for details.");
    }
}

void LogViewer::on_findLineEdit_returnPressed()
{
    if(gsFoundText != ui->findLineEdit->text()){
        giOcurrencesFound = 0;
        giLogCursorPos = 0;
        ui->plainTextEdit->moveCursor(QTextCursor::Start);
        gsFoundText = ui->findLineEdit->text();
        while(ui->plainTextEdit->find(ui->findLineEdit->text())){
            giOcurrencesFound ++;
        }
        ui->plainTextEdit->moveCursor(QTextCursor::Start);
        if(ui->plainTextEdit->find(ui->findLineEdit->text())) giLogCursorPos += 1;
        ui->ocurrencesCounterLabel->setText(QString("%1/%2").arg(giLogCursorPos).arg(giOcurrencesFound));
    }else{
        if(!ui->plainTextEdit->find(ui->findLineEdit->text())){
            ui->plainTextEdit->moveCursor(QTextCursor::Start);
            giLogCursorPos = 0;
            if(ui->plainTextEdit->find(ui->findLineEdit->text())) giLogCursorPos ++;
        }else{
            giLogCursorPos ++;
        }
        ui->ocurrencesCounterLabel->setText(QString("%1/%2").arg(giLogCursorPos).arg(giOcurrencesFound));
    }
}

void LogViewer::loadLogFile()
{
    QFile file("backup.log");
    if(file.exists()){
        if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
            return;

        QTextStream in(&file);
           while (!in.atEnd()) {
               QString line = in.readLine();
               ui->plainTextEdit->appendPlainText(line);
           }
    }
}
