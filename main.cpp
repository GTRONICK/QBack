#include "bumain.h"
#include <QApplication>
#include <QSplashScreen>

int main(int argc, char *argv[])
{
    QApplication a(argc,argv);
    QSplashScreen *splash = new QSplashScreen;
    splash->setPixmap(QPixmap(":/icons/QBackLogo.png"));
    splash->setWindowFlags(splash->windowFlags() | Qt::WindowStaysOnTopHint);
    Qt::Alignment topRight = Qt::AlignRight | Qt::AlignTop;    
    splash->show();
    a.processEvents();
    splash->showMessage(QObject::tr("Loading..."),topRight, Qt::white);
    a.processEvents();
    BUMain w;
    w.show();
    splash->finish(&w);
    return a.exec();
}
