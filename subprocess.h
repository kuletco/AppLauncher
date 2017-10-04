#ifndef SUBPROCESS_H
#define SUBPROCESS_H

#include <QObject>
#include <QDebug>
#include <QList>
#include <QUuid>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>


#ifdef Q_OS_WIN32
#include <windows.h>
#include <tlhelp32.h>
#endif

class SubProcess;
struct ProcInfo
{
    bool daemon;
    QUuid uuid;
    qint64 pid;
    qint32 exitcode;
    qint32 errorcode;
    SubProcess *subproc;

    ProcInfo()
    {
        pid = 0;
        exitcode =0;
        errorcode = 0;
        subproc = NULL;
    }

    inline bool operator ==(const ProcInfo &pi) const
    { return uuid == pi.uuid; }
};
Q_DECLARE_METATYPE(ProcInfo)

inline void registMetaType()
{
    qRegisterMetaType<ProcInfo>("ProcInfo");
    qRegisterMetaType<ProcInfo>("ProcInfo&");
    qRegisterMetaType< QList<ProcInfo> >("QList<ProcInfo>");
    qRegisterMetaType< QList<ProcInfo> >("QList<ProcInfo>&");
}

class SubProcess : public QObject
{
    Q_OBJECT
    QStringList subprocs;
    QProcess *process;
    QTimer *timer;
    ProcInfo procinfo;

public:
    SubProcess(ProcInfo &pi, QObject *parent = NULL);
    ~SubProcess();
    bool isRunning();
    static void msleep(qint32 msec);
    qint64 getPid();
    QString getEnv(const QString &name);
    QStringList getEnv();
    void setEnv(const QProcessEnvironment &env);
    void setEnv(const QString &name, const QString &value);

private:
    QString getParentPid(const QString &pid);
    void findChildrenProc(QStringList &pids, const QString pidstr);
    void stopChildrenProcs();

signals:
    void started(ProcInfo pi);
    void stoped(ProcInfo pi);
    void error(ProcInfo pi, QString errorStr);
    void message(ProcInfo pi, QString msg);

public slots:
    void run(const QString &cmd, bool daemon = false, bool force = false);
    void stop();

private slots:
    void slotError(QProcess::ProcessError err);
    void slotFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void slotStarted();
    void slotReadyRead();
    void updateSubProcList();
};

#endif // SUBPROCESS_H
