#include "translate.h"
#include "tempfilemanager.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icons/assets/app-icon2.png"));
    TempFileManager::cleanup();
    Translate w;
    w.show();
    return a.exec();
}
