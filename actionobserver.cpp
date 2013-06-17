#include "actionobserver.h"

#include <QDebug>

// Utility function for debug proposes
QString requestTypeToString(QMailServerRequestType t)
{
    switch(t)
    {
    case AcknowledgeNewMessagesRequestType:
        return QString("Acknowledging new messages");
    case TransmitMessagesRequestType:
        return QString("Transmitting new messages");
    case RetrieveFolderListRequestType:
        return QString("Retrieving a list of folders");
    case RetrieveMessageListRequestType:
        return QString("Retrieving a list of message");
    case RetrieveMessagesRequestType:
        return QString("Retrieving messages");
    case RetrieveMessagePartRequestType:
        return QString("Retrieving part of a message");
    case RetrieveMessageRangeRequestType:
        return QString("Retrieving a range of messages");
    case RetrieveMessagePartRangeRequestType:
        return QString("Retrieving parts of a messages");
    case RetrieveAllRequestType:
        return QString("Retrieving everything");
    case ExportUpdatesRequestType:
        return QString("Exporting updates");
    case SynchronizeRequestType:
        return QString("Synchronizing");
    case CopyMessagesRequestType:
        return QString("Copying messages");
    case MoveMessagesRequestType:
        return QString("Moving messages");
    case FlagMessagesRequestType:
        return QString("Flagging messages");
    case CreateFolderRequestType:
        return QString("Creating a folder");
    case RenameFolderRequestType:
        return QString("Renaming a folder");
    case DeleteFolderRequestType:
        return QString("Deleting a folder");
    case CancelTransferRequestType:
        return QString("Canceling a transfer");
    case DeleteMessagesRequestType:
        return QString("Deleteing a message");
    case SearchMessagesRequestType:
        return QString("Searching");
    case CancelSearchRequestType:
        return QString("Cancelling search");
    case ListActionsRequestType:
        return QString("Listing actions");
    case ProtocolRequestRequestType:
        return QString("Direct protocol request");
    case RetrieveNewMessagesRequestType:
        return QString("Retrieve new messages");
        // No default, to get warning when requests added
    }

    qWarning() << "Did not handle:" << t;
    return QString("Unknown/handled request.");
}

RunningAction::RunningAction(QSharedPointer<QMailActionInfo> action,
                             AccountsCache *accountsCache, QObject *parent) :
    QObject(parent),
    _runningInTransferEngine(false),
    _action(action),
    _accountsCache(accountsCache),
    _transferClient(new TransferEngineClient(this))
{
    _progress = 0.0;
    connect (_action.data(), SIGNAL(activityChanged(QMailServiceAction::Activity)), this, SLOT(activityChanged(QMailServiceAction::Activity)));
    connect (_action.data(), SIGNAL(statusAccountIdChanged(const QMailAccountId&)), this, SLOT(statusAccountIdChanged(const QMailAccountId&)));
    connect (_action.data(), SIGNAL(progressChanged(uint, uint)), this, SLOT(progressChanged(uint, uint)));
}

void RunningAction::activityChanged(QMailServiceAction::Activity activity)
{
    switch (activity) {
    case QMailServiceAction::Failed:
        // TODO: Where translations go ??
        if(_runningInTransferEngine)
            _transferClient->finishTransfer(_transferId, TransferEngineClient::TransferInterrupted, QString("Email Sync Failed"));
        _runningInTransferEngine = false;
        emit actionComplete(_action.data()->id());
        break;
    case QMailServiceAction::Successful:
        if(_runningInTransferEngine)
            _transferClient->finishTransfer(_transferId, TransferEngineClient::TransferFinished);
        _runningInTransferEngine = false;
        emit actionComplete(_action.data()->id());
        break;
    default:
        // we don't need to care about pending and in progress states
        break;
    }
}

void RunningAction::progressChanged(uint value, uint total)
{
    if (value < total) {
        qreal percent = ((value * 100) / total) * 0.01;
        // Avoid spamming transfer-ui
        if (percent > _progress + 0.05) {
            _progress = percent;
            if (_runningInTransferEngine) {
                _transferClient->updateTransferProgress(_transferId, _progress);
            }
        }
    }
}

void RunningAction::statusAccountIdChanged(const QMailAccountId &accountId)
{
    Q_ASSERT(accountId.toULongLong() <= 0);
    if (!_runningInTransferEngine) {
        AccountInfo acctInfo = _accountsCache->accountInfo(static_cast<Accounts::AccountId>(accountId.toULongLong()));
        _transferId = _transferClient->createSyncEvent(acctInfo.name, QUrl(), acctInfo.providerIcon);
        if (_transferId) {
            _runningInTransferEngine = true;
            _transferClient->startTransfer(_transferId);
        } else {
            qWarning() << Q_FUNC_INFO << "Failed to create sync event in transfer engine!";
        }
    } else {
        qWarning() << Q_FUNC_INFO << "This action is already running in the transfer engine!";
    }

}

ActionObserver::ActionObserver(QObject *parent) :
    QObject(parent),
    _accountsCache(new AccountsCache(this)),
    _actionObserver(new QMailActionObserver(this))
{
    connect(_actionObserver, SIGNAL(actionsChanged(QList<QSharedPointer<QMailActionInfo> >)),
        this, SLOT(actionsChanged(QList<QSharedPointer<QMailActionInfo> >)));
}

// Report only long sync type of actions.
// Small actions like exporting updates and flags are ignored, same for actions
// that only happen on email client UI is visible(dowload inline images, message parts, ...)
bool ActionObserver::isNotificationAction(QMailServerRequestType requestType)
{
    if (requestType == TransmitMessagesRequestType || requestType == RetrieveFolderListRequestType
            || requestType == RetrieveMessageListRequestType || requestType == RetrieveMessagesRequestType
            || requestType == RetrieveMessageRangeRequestType || requestType == RetrieveAllRequestType
            || requestType == SynchronizeRequestType || requestType == RetrieveNewMessagesRequestType) {
        return true;
    }
    return false;
}

// ################ Slots #####################

void ActionObserver::actionsChanged(QList<QSharedPointer<QMailActionInfo> > actionsList)
{
    foreach(QSharedPointer<QMailActionInfo> action, actionsList) {
        // discard actions already in the queue and fast actions to avoid spamming transfer-ui
        if(!_runningActions.contains(action.data()->id()) && isNotificationAction(action.data()->requestType())
                && !_completedActions.contains(action.data()->id())) {
            RunningAction* runningAction = new RunningAction(action, _accountsCache, this);
            _runningActions.insert(action.data()->id(), runningAction);
            connect(runningAction, SIGNAL(actionComplete(quint64)), this, SLOT(actionCompleted(quint64)));
        }
    }
    // Sometimes actionsChanged signals comes too late still containing actions that are already completed
    if (actionsList.size() == 0 && _completedActions.size() > 0)
        _completedActions.clear();
}

void ActionObserver::actionCompleted(quint64 id)
{
    Q_ASSERT(!_runningActions.contains(id));
    _runningActions.remove(id);
    _completedActions.append(id);
}



