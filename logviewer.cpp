#include "logviewer.h"
#include "ui_logviewer.h"
#include "styles.h"
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
    this->loadLogFile();
    Styles *lobStyle = new Styles;
    this->setStyleSheet(lobStyle->getElegantGnomeStyle());

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

}

void LogViewer::on_findLineEdit_returnPressed()
{
    if(!ui->plainTextEdit->find(ui->findLineEdit->text())){
        ui->plainTextEdit->moveCursor(QTextCursor::Start);
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
