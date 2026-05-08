#include "translate.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icons/assets/app-icon-1024.png"));
    Translate w;
    w.show();
    return a.exec();
}
