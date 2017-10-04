#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QDebug>
#include <QCoreApplication>

#include "settings.h"

class Controller : public QObject
{
    Q_OBJECT
    Settings *settings;
public:
    Controller(QObject *parent = nullptr);
    ~Controller();
    void run();

signals:

public slots:
};

#endif // CONTROLLER_H
