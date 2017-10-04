#ifndef SUBPROCMGR_H
#define SUBPROCMGR_H

#include <QObject>
#include <QList>
#include <QMetaType>

#include "subprocess.h"

class SubProcMgr : public QObject
{
    Q_OBJECT
    QList<ProcInfo> proclist;
    QProcessEnvironment gEnv;
public:
    SubProcMgr(QObject *parent = NULL);
    ~SubProcMgr();
    void setEnv(const QString &name, const QString &value);
    QUuid run(const QString &cmd, bool daemon = false, bool force = false);

private:
    void stopAndRemoveProcFromList(ProcInfo &pi, bool stopDaemon = false);

signals:
    void started(qint64 pid);
    void started(QUuid uuid);
    void stoped(qint64 pid, qint32 exitcode);
    void stoped(QUuid uuid, qint32 exitcode);
    void failed(qint64 pid, qint32 errorcode, QString errostr);
    void failed(QUuid uuid, qint32 errorcode, QString errostr);
    void allExited(qint32 lastcode);
    void message(qint64 pid, QString msg);
    void message(QUuid uuid, QString msg);

public slots:
    void run(QUuid uuid, const QString &cmd, bool daemon = false, bool force = false);
    void stop(qint64 pid);
    void stop(QUuid uuid);
    void stop(const QString &uuid);
    void stopAll();

private slots:
    void pStarted(ProcInfo pi);
    void pStoped(ProcInfo pi);
    void pError(ProcInfo pi, QString errorstr);
    void pMessage(ProcInfo pi, QString msg);
};

#endif // SUBPROCMGR_H
