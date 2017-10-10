#include "processmanager.h"

ProcessManager::ProcessManager(QObject *parent) : QObject(parent)
{
    proclist.clear();
    gEnv.clear();
}

ProcessManager::~ProcessManager()
{
    stopAll();
    proclist.clear();
}

void ProcessManager::setEnv(const QString &name, const QString &value)
{
    gEnv.insert(name, value);
}

QUuid ProcessManager::run(const QString &cmd, bool daemon, bool force)
{
    QUuid uuid = QUuid::createUuid();
    run(uuid, cmd, daemon, force);

    return uuid;
}

/* ========== PRIVATE ========== */
void ProcessManager::stopAndRemoveProcFromList(ProcInfo &pi, bool stopDaemon)
{
    if(pi.process)
    {
        if(stopDaemon)
            pi.process->stop();
        delete pi.process;
        pi.process = NULL;
    }

    if(proclist.contains(pi))
        proclist.removeOne(pi);
}

/* ========== PUBLIC SLOTS ========== */
void ProcessManager::run(QUuid uuid, const QString &cmd, bool daemon, bool force)
{
    if(uuid.isNull())
        return;

    ProcInfo pi;
    pi.uuid = uuid;
    pi.daemon = daemon;

    if(cmd.contains("cmd.exe") || cmd == "cmd")
#if QT_VERSION >= 0x050700
        pi.daemon = false;
#else
        pi.daemon = true;
#endif

    qDebug() << "Process UUID:" << pi.uuid.toString() << "CMD:" << cmd << "DAEMON:" << pi.daemon;

    pi.process = new Process(pi, this);
    connect(pi.process, SIGNAL(started(ProcInfo)), this, SLOT(pStarted(ProcInfo)));
    connect(pi.process, SIGNAL(stoped(ProcInfo)), this, SLOT(pStoped(ProcInfo)));
    connect(pi.process, SIGNAL(error(ProcInfo,QString)), this, SLOT(pError(ProcInfo,QString)));
    connect(pi.process, SIGNAL(message(ProcInfo,QString)), this, SLOT(pMessage(ProcInfo,QString)));

    if(!pi.daemon)
        proclist.append(pi);

    if(!gEnv.isEmpty())
        pi.process->setEnv(gEnv);

    pi.process->run(cmd, pi.daemon, force);
}

void ProcessManager::stop(qint64 pid)
{
    foreach (ProcInfo pi, proclist)
    {
        if(pi.pid == pid)
        {
            stopAndRemoveProcFromList(pi, true);
            break;
        }
    }
}

void ProcessManager::stop(QUuid uuid)
{
    foreach (ProcInfo pi, proclist)
    {
        if(pi.uuid == uuid)
        {
            stopAndRemoveProcFromList(pi, true);
            break;
        }
    }
}

void ProcessManager::stop(const QString &uuid)
{
    foreach (ProcInfo pi, proclist)
    {
        if(pi.uuid.toString() == uuid)
        {
            stopAndRemoveProcFromList(pi, true);
            break;
        }
    }
}

void ProcessManager::stopAll()
{
    foreach (ProcInfo pi, proclist)
    {
        stopAndRemoveProcFromList(pi);
    }
}

/* ========== PRIVATE SLOTS ========== */
void ProcessManager::pStarted(ProcInfo pi)
{
    qDebug() << "Process UUID:" << pi.uuid.toString() << "PID:" << pi.pid << "DAEMON:" << pi.daemon;

    emit started(pi.pid);
    emit started(pi.uuid);
}

void ProcessManager::pStoped(ProcInfo pi)
{
    qDebug() << "Process UUID:" << pi.uuid.toString() << "ExitCode:" << pi.exitcode;

    proclist.removeOne(pi);

    emit stoped(pi.pid, pi.exitcode);
    emit stoped(pi.uuid, pi.exitcode);

    if(proclist.size() == 0)
        emit allExited(pi.exitcode);
}

void ProcessManager::pError(ProcInfo pi, QString errorstr)
{
    qCritical() << "Process UUID:" << pi.uuid.toString() << "ErrorCode:" << pi.errorcode << errorstr;

    proclist.removeOne(pi);

    emit failed(pi.pid, pi.errorcode, errorstr);
    emit failed(pi.uuid, pi.errorcode, errorstr);

    if(proclist.size() == 0)
        emit allExited(pi.errorcode);
}

void ProcessManager::pMessage(ProcInfo pi, QString msg)
{
    //qDebug() << "Process UUID:" << pi.uuid.toString() << "MSG:" << msg;

    emit message(pi.pid, msg);
    emit message(pi.uuid, msg);
}
