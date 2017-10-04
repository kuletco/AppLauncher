#include "controller.h"

Controller::Controller(QObject *parent) : QObject(parent), settings(0)
{
    QFileInfo fileInfo(QCoreApplication::applicationFilePath());
    QString launcherName = fileInfo.baseName();
    qDebug() << "Launcher name:" << launcherName;

    settings = new Settings(this);
    settings->loadProfile();
}

Controller::~Controller()
{
    if(settings)
    {
        delete settings;
        settings = NULL;
    }
}

void Controller::run()
{
    exit(0);
}
