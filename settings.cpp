#include "settings.h"

Settings::Settings(QObject *parent) : QObject(parent), gSettings(0)
{
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
    if(gSettings->contains("icon"))
        icon = gSettings->value("icon").toString();
    if(gSettings->contains("path"))
        path = gSettings->value("path").toString();
    if(gSettings->contains("exec"))
        exec = gSettings->value("exec").toString();
    else
        return 3;
    gSettings->endGroup();

    icon = parseValue(icon);
    path = parseValue(path);
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
            args.append(pVal);
        }
    }

    qDebug() << "Application Settings:";
    qDebug() << QString(" name = %1").arg(name);
    qDebug() << QString(" desc = %1").arg(desc);
    qDebug() << QString(" icon = %1").arg(icon);
    qDebug() << QString(" path = %1").arg(path);
    qDebug() << QString(" exec = %1").arg(exec);
    qDebug() << QString(" args = %1").arg(args.join(" "));

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
