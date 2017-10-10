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
    bool gCreateDesktopIcon;
    bool gStartWithSystem;

public:
    Settings(QObject *parent = NULL);
    Settings(const QString &file, QObject *parent = NULL);
    ~Settings();
    qint32 getErrCode() { return errcode; }
    qint32 loadProfile(const QString &file = "");
    QString getAppName() { return name; }
    QString getAppDesc() { return desc; }
    QString getAppPath() { return path; }
    QString getAppIcon() { return icon; }
    QString getAppExec() { return exec; }
    QStringList getAppArgs() { return args; }
    bool isCreateDesktopIcon() { return gCreateDesktopIcon; }
    bool isStartWithSystem() { return gStartWithSystem; }

private:
    QString getDefaultProfileName();
    QString parseValue(const QString &value);
};

#endif // SETTINGS_H
