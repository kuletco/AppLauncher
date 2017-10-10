#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QCoreApplication>
#include <QDebug>
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

class Settings : public QObject
{
    Q_OBJECT
    qint32 errcode;
    QString gFile;
    QSettings *gSettings;
    QStringList gGroups;
    QString name;
    QString desc;
    QString path;
    QString icon;
    QString exec;
    QStringList args;

public:
    Settings(QObject *parent = NULL);
    Settings(const QString &file, QObject *parent = NULL);
    ~Settings();
    qint32 getErrCode() { return errcode; }
    qint32 loadProfile(const QString &file = "");

private:
    QString getDefaultProfileName();
    QString parseValue(const QString &value);
};

#endif // SETTINGS_H
