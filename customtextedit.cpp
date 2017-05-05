#include "customtextedit.h"


CustomTextEdit::CustomTextEdit(QWidget *parent):QPlainTextEdit(parent)
{

}

void CustomTextEdit::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void CustomTextEdit::dropEvent(QDropEvent *event)
{
    emit this->processDropEvent(event);
}

void CustomTextEdit::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}
