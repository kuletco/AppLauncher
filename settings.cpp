#include "settings.h"

Settings::Settings(QObject *parent) : QObject(parent), gSettings(0)
{
    gCreateDesktopIcon = false;
    gStartWithSystem = false;
    gFile = getDefaultProfileName();
}

Settings::Settings(const QString &file, QObject *parent) : QObject(parent), gSettings(0)
{
    gFile = file;
    errcode = loadProfile(file);
}

Settings::~Settings()
{
    if(gSettings)
    {
        delete gSettings;
        gSettings = NULL;
    }
}

qint32 Settings::loadProfile(const QString &file)
{
    if(!file.isEmpty())
    {
        if(QFile::exists(file))
            gFile = file;
        else
            return 1;
    }

    if(!QFile::exists(gFile))
        return 1;

    qDebug() << "Profile:" << gFile;

    gSettings = new QSettings(gFile, QSettings::IniFormat);
    gSettings->setIniCodec("UTF-8");

    gGroups = gSettings->childGroups();
    if(!gGroups.contains("Application"))
        return 2;

    gSettings->beginGroup("Application");
    if(gSettings->contains("name"))
        name = gSettings->value("name").toString();
    if(gSettings->contains("desc"))
        desc = gSettings->value("desc").toString();
    if(gSettings->contains("path"))
        path = gSettings->value("path").toString();
    if(gSettings->contains("icon"))
        icon = gSettings->value("icon").toString();
    if(gSettings->contains("exec"))
        exec = gSettings->value("exec").toString();
    else
        return 3;
    gSettings->endGroup();

    path = parseValue(path);
    icon = parseValue(icon);
    exec = parseValue(exec);

    icon = QDir::toNativeSeparators(path + QDir::toNativeSeparators("\\") + icon);
    exec = QDir::toNativeSeparators(path + QDir::toNativeSeparators("\\") + exec);

    if(gGroups.contains("Arguments"))
    {
        QStringList vals;
        gSettings->beginGroup("Arguments");
        foreach (QString key, gSettings->childKeys())
        {
            vals.append(gSettings->value(key).toString());
        }
        gSettings->endGroup();

        args.clear();
        foreach(QString val, vals)
        {
            QString pVal = parseValue(val);
            if(args.contains(pVal))
                continue;
            if(pVal.contains("="))
            {
                QStringList pVals = pVal.split("=");
                if(2 == pVals.size())
                {
                    pVals[1] = parseValue(pVals[1]);
                    pVal = pVals.join("=");
                }
            }
            args.append(pVal);
        }
    }

    if(gGroups.contains("Settings"))
    {
        gSettings->beginGroup("Settings");
        qDebug() << gSettings->value("CreateDesktopIcon").toString();
        if(gSettings->contains("CreateDesktopIcon"))
            gCreateDesktopIcon = gSettings->value("CreateDesktopIcon").toBool();
        if(gSettings->contains("StartWithSystem"))
            gStartWithSystem = gSettings->value("StartWithSystem").toBool();
        gSettings->endGroup();
    }

    qDebug() << "Application Settings:";
    qDebug() << QString(" Name = %1").arg(name);
    qDebug() << QString(" Desc = %1").arg(desc);
    qDebug() << QString(" Path = %1").arg(path);
    qDebug() << QString(" Icon = %1").arg(icon);
    qDebug() << QString(" Exec = %1").arg(exec);
    qDebug() << QString(" Args = %1").arg(args.join(" "));
    qDebug() << QString(" Create desktop icon: %1").arg(gCreateDesktopIcon?"true":"false");
    qDebug() << QString(" Startup with system: %1").arg(gStartWithSystem?"true":"false");

    return 0;
}

QString Settings::getDefaultProfileName()
{
    QFileInfo fi(QCoreApplication::applicationFilePath());
    return QDir::toNativeSeparators(fi.absolutePath() + "\\" + fi.baseName() + ".ini");
}

QString Settings::parseValue(const QString &value)
{
    if(!value.startsWith("$"))
        return value;

    QStringList list = value.split(QDir::toNativeSeparators("\\"));
    if(list.size() < 2)
        return value;

    QString section = list.takeFirst().remove("$");
    QString keyname = list.takeFirst();
    QString string = "";
    QString val = "";

    if(!list.isEmpty())
        string = QDir::toNativeSeparators(list.join("\\"));

    if(!gGroups.contains(section))
        return value;

    gSettings->beginGroup(section);
    if(!gSettings->contains(keyname))
        return value;
    val = parseValue(gSettings->value(keyname).toString());
    gSettings->endGroup();

    if(val.isEmpty())
        return string;

    if(!string.isEmpty())
    {
        val = val + QDir::toNativeSeparators("\\");
        val = val + string;
    }

    return QDir::toNativeSeparators(val);
}
