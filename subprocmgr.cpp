#include "subprocmgr.h"

SubProcMgr::SubProcMgr(QObject *parent) : QObject(parent)
{
    proclist.clear();
    gEnv.clear();
}

SubProcMgr::~SubProcMgr()
{
    stopAll();
    proclist.clear();
}

void SubProcMgr::setEnv(const QString &name, const QString &value)
{
    gEnv.insert(name, value);
}

QUuid SubProcMgr::run(const QString &cmd, bool daemon, bool force)
{
    QUuid uuid = QUuid::createUuid();
    run(uuid, cmd, daemon, force);

    return uuid;
}

/* ========== PRIVATE ========== */
void SubProcMgr::stopAndRemoveProcFromList(ProcInfo &pi, bool stopDaemon)
{
    if(pi.subproc)
    {
        if(stopDaemon)
            pi.subproc->stop();
        delete pi.subproc;
        pi.subproc = NULL;
    }

    if(proclist.contains(pi))
        proclist.removeOne(pi);
}

/* ========== PUBLIC SLOTS ========== */
void SubProcMgr::run(QUuid uuid, const QString &cmd, bool daemon, bool force)
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

    pi.subproc = new SubProcess(pi, this);
    connect(pi.subproc, SIGNAL(started(ProcInfo)), this, SLOT(pStarted(ProcInfo)));
    connect(pi.subproc, SIGNAL(stoped(ProcInfo)), this, SLOT(pStoped(ProcInfo)));
    connect(pi.subproc, SIGNAL(error(ProcInfo,QString)), this, SLOT(pError(ProcInfo,QString)));
    connect(pi.subproc, SIGNAL(message(ProcInfo,QString)), this, SLOT(pMessage(ProcInfo,QString)));

    if(!pi.daemon)
        proclist.append(pi);

    if(!gEnv.isEmpty())
        pi.subproc->setEnv(gEnv);

    pi.subproc->run(cmd, pi.daemon, force);
}

void SubProcMgr::stop(qint64 pid)
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

void SubProcMgr::stop(QUuid uuid)
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

void SubProcMgr::stop(const QString &uuid)
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

void SubProcMgr::stopAll()
{
    foreach (ProcInfo pi, proclist)
    {
        stopAndRemoveProcFromList(pi);
    }
}

/* ========== PRIVATE SLOTS ========== */
void SubProcMgr::pStarted(ProcInfo pi)
{
    qDebug() << "Process UUID:" << pi.uuid.toString() << "PID:" << pi.pid << "DAEMON:" << pi.daemon;

    emit started(pi.pid);
    emit started(pi.uuid);
}

void SubProcMgr::pStoped(ProcInfo pi)
{
    qDebug() << "Process UUID:" << pi.uuid.toString() << "ExitCode:" << pi.exitcode;

    proclist.removeOne(pi);

    emit stoped(pi.pid, pi.exitcode);
    emit stoped(pi.uuid, pi.exitcode);

    if(proclist.size() == 0)
        emit allExited(pi.exitcode);
}

void SubProcMgr::pError(ProcInfo pi, QString errorstr)
{
    qCritical() << "Process UUID:" << pi.uuid.toString() << "ErrorCode:" << pi.errorcode << errorstr;

    proclist.removeOne(pi);

    emit failed(pi.pid, pi.errorcode, errorstr);
    emit failed(pi.uuid, pi.errorcode, errorstr);

    if(proclist.size() == 0)
        emit allExited(pi.errorcode);
}

void SubProcMgr::pMessage(ProcInfo pi, QString msg)
{
    //qDebug() << "Process UUID:" << pi.uuid.toString() << "MSG:" << msg;

    emit message(pi.pid, msg);
    emit message(pi.uuid, msg);
}
