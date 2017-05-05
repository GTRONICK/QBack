#ifndef CUSTOMTEXTEDIT_H
#define CUSTOMTEXTEDIT_H

#include <QObject>
#include <QWidget>
#include <QPlainTextEdit>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>

class CustomTextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    CustomTextEdit(QWidget *parent);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);

signals:
    void processDropEvent(QDropEvent *event);
};

#endif // CUSTOMTEXTEDIT_H
