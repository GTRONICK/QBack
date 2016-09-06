#include "bumain.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    BUMain w;
    w.show();
    return a.exec();
}
