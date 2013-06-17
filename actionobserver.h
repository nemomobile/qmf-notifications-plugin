#ifndef ACTIONOBSERVER_H
#define ACTIONOBSERVER_H

#include "accountscache.h"

#include <QObject>
#include <transferengineclient.h>
#include <qmailserviceaction.h>

class RunningAction : public QObject
{
    Q_OBJECT
public:
    explicit RunningAction(QSharedPointer<QMailActionInfo> action,
                           AccountsCache *accountsCache, QObject *parent = 0);

private slots:
    void activityChanged(QMailServiceAction::Activity activity);
    void progressChanged(uint value, uint total);
    void statusAccountIdChanged(const QMailAccountId &accountId);

signals:
    void actionComplete(quint64 id);

private:
    qreal _progress;
    int _transferId;
    bool _runningInTransferEngine;
    QSharedPointer<QMailActionInfo> _action;
    AccountsCache *_accountsCache;
    TransferEngineClient *_transferClient;
};

class ActionObserver : public QObject
{
    Q_OBJECT
public:
    explicit ActionObserver(QObject *parent = 0);

    AccountsCache *_accountsCache;

private slots:
    void actionsChanged(QList<QSharedPointer<QMailActionInfo> > actions);
    void actionCompleted(quint64 id);

private:
    bool isNotificationAction(QMailServerRequestType requestType);

    QMailActionObserver *_actionObserver;
    QList<quint64> _completedActions;
    QHash<quint64, RunningAction*> _runningActions;
};

#endif // ACTIONOBSERVER_H
