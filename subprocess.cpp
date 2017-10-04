#include "subprocess.h"

#define UPDATE_SUBPROCS_TIME 2000
#define STOP_SUBPROCS_WAIT 1000

SubProcess::SubProcess(ProcInfo &pi, QObject *parent) : QObject(parent), process(0), timer(0)
{
    procinfo = pi;
    procinfo.errorcode = 0;
    procinfo.exitcode = 0;
    procinfo.pid = 0;

    process = new QProcess(this);
    connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(slotError(QProcess::ProcessError)));
    connect(process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotFinished(int,QProcess::ExitStatus)));
    connect(process, SIGNAL(started()), this, SLOT(slotStarted()));
    connect(process, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateSubProcList()));
}

SubProcess::~SubProcess()
{
    if(timer)
    {
        if(timer->isActive())
            timer->stop();
        delete timer;
        timer = NULL;
    }
    if(process)
    {
        if(QProcess::Running == process->state() && !procinfo.daemon)
            process->close();
        delete process;
        process = NULL;
    }
}

bool SubProcess::isRunning()
{
    if(process && QProcess::Running == process->state())
        return true;
    else
        return false;
}

void SubProcess::msleep(qint32 msec)
{
    QMutex mutex;
    mutex.lock();
    QWaitCondition wait;
    wait.wait(&mutex, msec);
    mutex.unlock();
}

qint64 SubProcess::getPid()
{
    if(!isRunning())
        return -1;

#if QT_VERSION >= 0x050300
    return process->processId();
#elif defined Q_OS_WIN32
    return process->pid()->dwProcessId;
#else
    return process->pid();
#endif
}

QString SubProcess::getEnv(const QString &name)
{
    QString envstr = "";

    if(process)
    {
        QProcessEnvironment env = process->processEnvironment();
        envstr = env.value(name);
    }

    return envstr;
}

QStringList SubProcess::getEnv()
{
    QStringList envs;
    envs.clear();

    if(process)
    {
        QProcessEnvironment env = process->processEnvironment();
        envs = env.toStringList();
    }

    return envs;
}

void SubProcess::setEnv(const QProcessEnvironment &env)
{
    if(!process)
        return;

    QProcessEnvironment pEnv = process->processEnvironment();
    QStringList keys = env.keys();
    foreach (QString key, keys)
    {
        pEnv.insert(key, env.value(key));
    }
    process->setProcessEnvironment(pEnv);
}

void SubProcess::setEnv(const QString &name, const QString &value)
{
    if(!process)
        return;

    QProcessEnvironment env = process->processEnvironment();
    env.insert(name, value);
    process->setProcessEnvironment(env);
}

/* ========== PRIVATE ========== */
QString SubProcess::getParentPid(const QString &pid)
{
    QString ppid = QString::Null();

    if(!process)
        return ppid;

    if(pid.isEmpty())
        return ppid;

#if defined Q_OS_LINUX
    QString statfile = QString("/proc/%1/stat").arg(pid);
    QFile file(statfile);
    if(!file.exists())
        return QString::Null();
    if(!file.open(QIODevice::ReadOnly|QIODevice::Text))
        return QString::Null();

    QString contents;
    contents = file.readAll();
    file.close();

    if(contents.isEmpty())
        return QString::Null();

    QStringList list = contents.split(" ");
    ppid = list.at(3);
#elif defined Q_OS_WIN32
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe;
    memset(&pe, 0, sizeof(PROCESSENTRY32));
    pe.dwSize = sizeof(PROCESSENTRY32);

    if(Process32First(h, &pe))
    {
        do {
            if(pe.th32ProcessID == process->pid()->dwProcessId)
            {
                ppid.setNum(pe.th32ParentProcessID);
                break;
            }
        } while(Process32Next(h, &pe));
    }

    CloseHandle(h);
#endif

    return ppid;
}

void SubProcess::findChildrenProc(QStringList &pids, const QString pidstr)
{
    if(!process)
        return;

    if(pids.isEmpty())
        return;

    QListIterator<QString> iter(pids);
    while(iter.hasNext())
    {
        QString pid = iter.next();
        if(pidstr == getParentPid(pid))
        {
            if(!subprocs.contains(pid))
            {
                subprocs.append(pid);
                findChildrenProc(pids, pid);
            }
        }
    }
}

void SubProcess::stopChildrenProcs()
{
    if(!isRunning())
        return;

#if defined Q_OS_LINUX
    QString pids = subprocs.join(" ");
    QString cmd = QString("kill %1 %2").arg("-SIGTERM").arg(pids);
#elif defined Q_OS_WIN32
    QString cmd = QString("taskkill.exe /T /F /PID %1").arg(QString::number(getPid()));
#endif

    QProcess::execute(cmd);
}

/* ========== PUBLIC SLOTS ========== */
void SubProcess::run(const QString &cmd, bool daemon, bool force)
{
    if(cmd.isEmpty())
        return;

    QStringList args;
    QStringList cmdlist = cmd.split(" ");
    QString bin = cmdlist.takeFirst();

    args.clear();

#if defined Q_OS_WIN32 && QT_VERSION >= 0x050700
    if(bin.contains("cmd.exe") || bin == "cmd")
    {
        process->setCreateProcessArgumentsModifier([] (QProcess::CreateProcessArguments *args)
        {
            args->flags |= CREATE_NEW_CONSOLE;
            args->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;
            args->startupInfo->dwFlags |= STARTF_USEFILLATTRIBUTE;
            args->startupInfo->dwFillAttribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        });
        args.append("/k");
    }
#endif

    args.append(cmdlist);

    if(daemon)
    {
        process->startDetached(cmd);
        return;
    }

    if(QProcess::NotRunning != process->state() && force == false)
        return;

    stop();
    process->start(bin, args);
}

void SubProcess::stop()
{
    if(!process)
        return;

    if(QProcess::NotRunning == process->state() || procinfo.daemon)
        return;

    stopChildrenProcs();

    // Wait for children process exit it self.
    msleep(STOP_SUBPROCS_WAIT);

    if(QProcess::Running == process->state())
    {
        process->terminate();
        if(QProcess::NotRunning != process->state())
            process->kill();

        process->close();
    }
}

/* ========== PRIVATE SLOTS ========== */
void SubProcess::slotError(QProcess::ProcessError err)
{
    Q_UNUSED(err)

    if(timer && timer->isActive())
        timer->stop();

    procinfo.errorcode = err;

    emit error(procinfo, process->errorString());
}

void SubProcess::slotFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if(timer && timer->isActive())
        timer->stop();

    if(QProcess::NormalExit == exitStatus)
        procinfo.exitcode = exitCode;
    else
        procinfo.exitcode = procinfo.errorcode;

    emit stoped(procinfo);
}

void SubProcess::slotStarted()
{
/* Update subprocess list only needed for linux,
 * because taskkill /t can killall the subprocesses at a time */
#ifdef Q_OS_LINUX
    if(timer && !timer->isActive())
        timer->start(UPDATE_SUBPROCS_TIME);
#endif

    procinfo.pid = getPid();

    emit started(procinfo);
}

void SubProcess::slotReadyRead()
{
    QString msg = process->readAll();
    //qDebug() << "PROC" << procinfo.pid << "MSG:" << msg;

    emit message(procinfo, msg);
}

void SubProcess::updateSubProcList()
{
#if defined Q_OS_LINUX
    QString pidstr = QString::number(getPid());
    QStringList pidlist;
    pidlist.clear();

    QDir dir = QDir("/proc");
    QStringList dirlist = dir.entryList(QStringList(""), QDir::AllDirs, QDir::Name);

    pidlist = dirlist.filter(QRegExp("[0-9].*"));

    findChildrenProc(pidlist, pidstr);
#elif defined Q_OS_WIN32
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe;
    memset(&pe, 0, sizeof(PROCESSENTRY32));
    pe.dwSize = sizeof(PROCESSENTRY32);

    if(Process32First(h, &pe))
    {
        do {
            if(pe.th32ParentProcessID == process->pid()->dwProcessId)
            {
                subprocs.append(QString::number(pe.th32ProcessID));
            }
        } while(Process32Next(h, &pe));
    }

    CloseHandle(h);
#endif
}
