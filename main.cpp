#include "bumain.h"
#include <QApplication>
#include <QSplashScreen>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QSplashScreen *splash = new QSplashScreen;
    splash->setPixmap(QPixmap(":/icons/splash.PNG"));
    splash->show();
    Qt::Alignment topRight = Qt::AlignRight | Qt::AlignTop;
    splash->showMessage(QObject::tr("Loading..."),
    topRight, Qt::white);
    BUMain w;
    w.show();

    splash->finish(&w);
    delete splash;

    return a.exec();
}
