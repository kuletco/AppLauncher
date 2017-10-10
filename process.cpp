#include "process.h"

#define UPDATE_SUBPROCS_TIME 2000
#define STOP_SUBPROCS_WAIT 1000

Process::Process(ProcInfo &pi, QObject *parent) : QObject(parent), gProcess(0), timer(0)
{
    procinfo = pi;
    procinfo.errorcode = 0;
    procinfo.exitcode = 0;
    procinfo.pid = 0;

    gProcess = new QProcess(this);
    connect(gProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(slotError(QProcess::ProcessError)));
    connect(gProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotFinished(int,QProcess::ExitStatus)));
    connect(gProcess, SIGNAL(started()), this, SLOT(slotStarted()));
    connect(gProcess, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateSubProcList()));
}

Process::~Process()
{
    if(timer)
    {
        if(timer->isActive())
            timer->stop();
        delete timer;
        timer = NULL;
    }
    if(gProcess)
    {
        if(QProcess::Running == gProcess->state() && !procinfo.daemon)
            gProcess->close();
        delete gProcess;
        gProcess = NULL;
    }
}

bool Process::isRunning()
{
    if(gProcess && QProcess::Running == gProcess->state())
        return true;
    else
        return false;
}

void Process::msleep(qint32 msec)
{
    QMutex mutex;
    mutex.lock();
    QWaitCondition wait;
    wait.wait(&mutex, msec);
    mutex.unlock();
}

qint64 Process::getPid()
{
    if(!isRunning())
        return -1;

#if QT_VERSION >= 0x050300
    return gProcess->processId();
#elif defined Q_OS_WIN32
    return gProcess->pid()->dwProcessId;
#else
    return gProcess->pid();
#endif
}

QString Process::getEnv(const QString &name)
{
    QString envstr = "";

    if(gProcess)
    {
        QProcessEnvironment env = gProcess->processEnvironment();
        envstr = env.value(name);
    }

    return envstr;
}

QStringList Process::getEnv()
{
    QStringList envs;
    envs.clear();

    if(gProcess)
    {
        QProcessEnvironment env = gProcess->processEnvironment();
        envs = env.toStringList();
    }

    return envs;
}

void Process::setEnv(const QProcessEnvironment &env)
{
    if(!gProcess)
        return;

    QProcessEnvironment pEnv = gProcess->processEnvironment();
    QStringList keys = env.keys();
    foreach (QString key, keys)
    {
        pEnv.insert(key, env.value(key));
    }
    gProcess->setProcessEnvironment(pEnv);
}

void Process::setEnv(const QString &name, const QString &value)
{
    if(!gProcess)
        return;

    QProcessEnvironment env = gProcess->processEnvironment();
    env.insert(name, value);
    gProcess->setProcessEnvironment(env);
}

/* ========== PRIVATE ========== */
QString Process::getParentPid(const QString &pid)
{
    QString ppid = QString::Null();

    if(!gProcess)
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
            if(pe.th32ProcessID == gProcess->pid()->dwProcessId)
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

void Process::findChildrenProc(QStringList &pids, const QString pidstr)
{
    if(!gProcess)
        return;

    if(pids.isEmpty())
        return;

    QListIterator<QString> iter(pids);
    while(iter.hasNext())
    {
        QString pid = iter.next();
        if(pidstr == getParentPid(pid))
        {
            if(!gProcList.contains(pid))
            {
                gProcList.append(pid);
                findChildrenProc(pids, pid);
            }
        }
    }
}

void Process::stopChildrenProcs()
{
    if(!isRunning())
        return;

#if defined Q_OS_LINUX
    QString pids = gProcList.join(" ");
    QString cmd = QString("kill %1 %2").arg("-SIGTERM").arg(pids);
#elif defined Q_OS_WIN32
    QString cmd = QString("taskkill.exe /T /F /PID %1").arg(QString::number(getPid()));
#endif

    QProcess::execute(cmd);
}

/* ========== PUBLIC SLOTS ========== */
void Process::run(const QString &cmd, bool daemon, bool force)
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
        gProcess->setCreateProcessArgumentsModifier([] (QProcess::CreateProcessArguments *args)
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
        gProcess->startDetached(cmd);
        return;
    }

    if(QProcess::NotRunning != gProcess->state() && force == false)
        return;

    stop();
    gProcess->start(bin, args);
}

void Process::stop()
{
    if(!gProcess)
        return;

    if(QProcess::NotRunning == gProcess->state() || procinfo.daemon)
        return;

    stopChildrenProcs();

    // Wait for children gProcess exit it self.
    msleep(STOP_SUBPROCS_WAIT);

    if(QProcess::Running == gProcess->state())
    {
        gProcess->terminate();
        if(QProcess::NotRunning != gProcess->state())
            gProcess->kill();

        gProcess->close();
    }
}

/* ========== PRIVATE SLOTS ========== */
void Process::slotError(QProcess::ProcessError err)
{
    Q_UNUSED(err)

    if(timer && timer->isActive())
        timer->stop();

    procinfo.errorcode = err;

    emit error(procinfo, gProcess->errorString());
}

void Process::slotFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if(timer && timer->isActive())
        timer->stop();

    if(QProcess::NormalExit == exitStatus)
        procinfo.exitcode = exitCode;
    else
        procinfo.exitcode = procinfo.errorcode;

    emit stoped(procinfo);
}

void Process::slotStarted()
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

void Process::slotReadyRead()
{
    QString msg = gProcess->readAll();
    //qDebug() << "PROC" << procinfo.pid << "MSG:" << msg;

    emit message(procinfo, msg);
}

void Process::updateSubProcList()
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
            if(pe.th32ParentProcessID == gProcess->pid()->dwProcessId)
            {
                gProcList.append(QString::number(pe.th32ProcessID));
            }
        } while(Process32Next(h, &pe));
    }

    CloseHandle(h);
#endif
}
