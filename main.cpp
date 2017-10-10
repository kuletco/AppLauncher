#include <QCoreApplication>
#include <QDebug>
#ifdef _WIN32
#include <windows.h>
#endif

#include "controller.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

#ifdef _WIN32
    if(AttachConsole(ATTACH_PARENT_PROCESS))
    {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

    Controller controller;
    controller.run();

    return a.exec();
}
