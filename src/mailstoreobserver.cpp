/*
 * Copyright (C) 2013-2014 Jolla Ltd.
 * Contact: Valerio Valerio <valerio.valerio@jollamobile.com>
 *
 * This file is part of qmf-notifications-plugin
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

#include "mailstoreobserver.h"

// Qt
#include <QDBusConnection>
#include <QDebug>

namespace {

QVariant remoteAction(const QString &name, const QString &displayName, const QString &method, const QVariantList &arguments = QVariantList())
{
    return Notification::remoteAction(name, displayName, "com.jolla.email.ui", "/com/jolla/email/ui", "com.jolla.email.ui", method, arguments);
}

}

MailStoreObserver::MailStoreObserver(QObject *parent) :
    QObject(parent),
    _newMessagesCount(0),
    _publishedItemCount(0),
    _oldMessagesCount(0),
    _appOnScreen(false),
    _replacesId(0),
    _notification(new Notification(this)),
    _lastReceivedId(0)
{
    qDebug() << "Store observer initialized";
    _storage = QMailStore::instance();
    _notification->setCategory("x-nemo.email");

    connect(_storage, SIGNAL(messagesAdded(const QMailMessageIdList&)),
            this, SLOT(addMessages(const QMailMessageIdList&)));
    connect(_storage, SIGNAL(messagesUpdated(const QMailMessageIdList&)),
            this, SLOT(updateMessages(const QMailMessageIdList&)));
    connect(_storage, SIGNAL(messagesRemoved(const QMailMessageIdList&)),
            this, SLOT(removeMessages(const QMailMessageIdList&)));

    // If something goes wrong (e.g crash), we keep proper states,
    // if there is a published notification it will be reformated.
    if (publishedNotification()) {
        if (!_publishedMessages.isEmpty()) {
            qDebug() << "Reformating published messages " <<  _publishedMessages;
            reformatPublishedMessages();
        } else {
            closeNotifications();
        }
    } else {
        qDebug() << "No published notification!";
    }

    QDBusConnection::sessionBus().connect(QString(), "/com/jolla/email/ui", "com.jolla.email.ui", "displayEntered",
                                          this, SLOT(setNotifyOff()));
    QDBusConnection::sessionBus().connect(QString(), "/com/jolla/email/ui", "com.jolla.email.ui", "displayExit",
                                          this, SLOT(setNotifyOn()));
}

// Close existent notification
void MailStoreObserver::closeNotifications()
{
    qDebug() << "Closing notification " << _replacesId;
    _notification->close();
    _publishedItemCount = 0;
    _newMessagesCount = 0;
    _oldMessagesCount = 0;
    _publishedMessageList.clear();
}

// Contructs messageInfo object from a email message
MessageInfo* MailStoreObserver::constructMessageInfo(const QMailMessageMetaData &message)
{
    MessageInfo* messageInfo = new MessageInfo();
    messageInfo->id = message.id();
    if (message.from().name().isEmpty()) {
        messageInfo->sender = message.from().toString();
    } else {
        messageInfo->sender = message.from().name();
    }
    messageInfo->subject = message.subject();
    messageInfo->timeStamp = message.date().toUTC();
    messageInfo->accountId = message.parentAccountId();

    return messageInfo;
}

QDateTime MailStoreObserver::lastestPublishedMessageTimeStamp()
{
    QDateTime lastestTimeStamp;
    foreach (QSharedPointer<MessageInfo> msgInfo, _publishedMessageList) {
        if (msgInfo->timeStamp >= lastestTimeStamp) {
            lastestTimeStamp = msgInfo->timeStamp;
        }
    }
    return lastestTimeStamp;
}

// Returns messageInfo object from the list of published messages
QSharedPointer<MessageInfo> MailStoreObserver::messageInfo(const QMailMessageId id)
{
    if (_publishedMessageList.size()) {
        // Get one value
        if (!id.isValid()) {
            foreach (QSharedPointer<MessageInfo> msgInfo, _publishedMessageList)
                return msgInfo;
        } else {
            Q_ASSERT(_publishedMessageList.contains(id));
            return _publishedMessageList.value(id);
        }
    }
    return QSharedPointer<MessageInfo>();
}

// Check if we have messages from more than one account
bool MailStoreObserver::notificationsFromMultipleAccounts()
{
    uint accountId = 0;
    foreach (QSharedPointer<MessageInfo> msgInfo, _publishedMessageList) {
        if (accountId == 0) {
            accountId = msgInfo->accountId.toULongLong();
        } else {
            if (accountId != msgInfo->accountId.toULongLong()) {
                return true;
            }
        }
    }
    return false;
}

// Check if this message should be notified, old messages are not
// notified since QMailMessage::NoNotification is used
bool MailStoreObserver::notifyMessage(const QMailMessageMetaData &message)
{
    if (message.messageType()==QMailMessage::Email &&
        !(message.status() & QMailMessage::Read) &&
        !(message.status() & QMailMessage::Temporary)&&
        !(message.status() & QMailMessage::NoNotification) &&
        !(message.status() & QMailMessage::Trash)) {
        return true;
    } else {
        return false;
    }
}

// App is on screen beep only
void MailStoreObserver::notifyOnly()
{
    _notification->setPreviewSummary(QString());
    _notification->setPreviewBody(QString());
    _notification->setSummary(QString());
    _notification->setBody(QString());
    _notification->publish();
    // Close notification and reset the counters
    closeNotifications();
}

void MailStoreObserver::reformatNotification(bool showPreview, int newCount)
{
    // All messages read remove notification
    if (newCount == 0) {
        closeNotifications();
        // App is on screen
    } else if (_appOnScreen) {
        // We got need messages, beep only
        if (showPreview) {
            notifyOnly();
        } else {
            closeNotifications();
        }
    } else {
        publishNotifications(showPreview, newCount);
        _notification->setItemCount(newCount);
        _notification->setHintValue("x-nemo.email.published-messages", publishedMessageIds());
        _notification->publish();

        // Reset count
        _oldMessagesCount = 0;
        _newMessagesCount = 0;

        _publishedItemCount = _publishedMessageList.size();
    }
}

void MailStoreObserver::reformatPublishedMessages()
{
    _enabledAccounts = QMailStore::instance()->queryAccounts(QMailAccountKey::messageType(QMailMessage::Email)
                                                             & QMailAccountKey::status(QMailAccount::Enabled));

    QStringList messageIdList = _publishedMessages.split(",", QString::SkipEmptyParts);
    for (int i = 0; i < messageIdList.size(); ++i) {
        QMailMessageId messageId(messageIdList.at(i).toInt());
        QMailMessageMetaData message(messageId);
        QMailAccountId accountId(message.parentAccountId());
        // Checks if parent account is still valid
        // accounts can be removed when messageServer is not running.
        if (_enabledAccounts.contains(accountId)) {
            if (notifyMessage(message)) {
                _publishedMessageList.insert(messageId,QSharedPointer<MessageInfo>(constructMessageInfo(message)));
            } else {
                // This message got read somewhere else
                 _publishedItemCount--;
            }
        }
    }
    reformatNotification(false, _publishedMessageList.size());
}

QString MailStoreObserver::publishedMessageIds()
{
    Q_ASSERT(_publishedMessageList.size());
    QStringList messageIdList;
    QList<QMailMessageId> messageIds = _publishedMessageList.uniqueKeys();
    foreach (const QMailMessageId &id, messageIds) {
       messageIdList << QString::number(id.toULongLong());
    }
    return messageIdList.join(",");
}

void MailStoreObserver::publishNotifications(bool showPreview, int newCount)
{
    if (newCount == 1) {
        QSharedPointer<MessageInfo> msgInfo;
        if (_newMessagesCount) {
            msgInfo = messageInfo(_lastReceivedId);
        } else {
            if (_publishedMessageList.size() > 1) {
                qWarning() << Q_FUNC_INFO << "Published message list contains more than one item!";
            }
            msgInfo = messageInfo();
        }

        if (!msgInfo.isNull()) {
            QVariant acctId = static_cast<int>(msgInfo.data()->id.toULongLong());

            // If is a new message just added, notify the user.
            if (showPreview && !_appOnScreen) {
                _notification->setPreviewBody(msgInfo.data()->subject);
                _notification->setPreviewSummary(msgInfo.data()->sender);
            } else {
                _notification->setPreviewSummary(QString());
            }
            _notification->setSummary(msgInfo.data()->sender);
            _notification->setBody(msgInfo.data()->subject);
            _notification->setTimestamp(msgInfo.data()->timeStamp);
            _notification->setRemoteAction(::remoteAction("default", "", "openMessage", QVariantList() << acctId));
        } else {
            qWarning() << Q_FUNC_INFO << "Failed to publish notification, invalid message info";
        }
    } else {
        //: Summary of new email(s) notification
        //% "You have %n new email(s)"
        QString summary = qtTrId("qmf-notification_new_email_notification", newCount);
        _notification->setSummary(summary);
        if (showPreview  && !_appOnScreen) {
            //: Notification preview of new email(s)
            //% "You have %n new email(s)"
            QString previewSummary = qtTrId("qmf-notification_new_email_banner_notification", _newMessagesCount);
            _notification->setPreviewSummary(previewSummary);
            _notification->setPreviewBody(QString());
        } else {
            _notification->setPreviewSummary(QString());
        }
        _notification->setBody(QString());
        _notification->setTimestamp(lastestPublishedMessageTimeStamp());

        if (notificationsFromMultipleAccounts()) {
            _notification->setRemoteAction(::remoteAction("default", "", "openCombinedInbox"));
        } else {
            QSharedPointer<MessageInfo> msgInfo = messageInfo();
            QVariant acctId;
            if (!msgInfo.isNull()) {
                acctId = static_cast<int>(msgInfo.data()->accountId.toULongLong());
            } else {
                qWarning() << Q_FUNC_INFO << "Failed to get account information, invalid message info";
                acctId = 0;
            }
            _notification->setRemoteAction(::remoteAction("default", "", "openInbox", QVariantList() << acctId));
        }
    }
}

// Checks if there is a published notification for this app with hint value
// 'x-nemo.email.published-messages'
bool MailStoreObserver::publishedNotification()
{
    QList<QObject*> publishedNotifications = _notification->notifications();

    if (publishedNotifications.size()) {
        for (int i = 0; i < publishedNotifications.size(); i++) {
            Notification *publishedNotification = static_cast<Notification*>(publishedNotifications.at(i));
            QString publishedMessages = publishedNotification->hintValue("x-nemo.email.published-messages").toString();
            if (!publishedMessages.isEmpty()) {
                _replacesId = publishedNotification->replacesId();
                _publishedItemCount = publishedNotification->itemCount();
                _publishedMessages = publishedMessages;
                _notification->setReplacesId(_replacesId);
                qDeleteAll(publishedNotifications);
                return true;
            }
        }
    }
    _publishedItemCount = 0;
    qDeleteAll(publishedNotifications);
    return false;
}

// ################ Slots #####################

void MailStoreObserver::actionsCompleted()
{
    if (_oldMessagesCount > 0 || _newMessagesCount > 0) {
        // if there's already a published notification use it
        publishedNotification();

        if (_oldMessagesCount > _publishedItemCount) {
            qWarning() << "Old message count is bigger than current published items count, reseting counter: old:"
                       << _oldMessagesCount << " Published:" << _publishedItemCount << " New:" << _newMessagesCount;
        }
        reformatNotification(_newMessagesCount ? true : false, _publishedMessageList.size());
    }
}

void MailStoreObserver::addMessages(const QMailMessageIdList &ids)
{
    foreach (const QMailMessageId &id, ids) {
        QMailMessageMetaData message(id);
        // Workaround for plugin that try to add same message twice
        if (notifyMessage(message) && !_publishedMessageList.contains(id)) {
            _newMessagesCount++;
            _publishedMessageList.insert(id,QSharedPointer<MessageInfo>(constructMessageInfo(message)));
            _lastReceivedId = id;
        }
    }
}

void MailStoreObserver::removeMessages(const QMailMessageIdList &ids)
{
    foreach (const QMailMessageId &id, ids) {
        if (_publishedMessageList.contains(id)) {
            _oldMessagesCount++;
            _publishedMessageList.remove(id);
        }
    }
    // Local action not handled by action observer
    actionsCompleted();
}

void MailStoreObserver::updateMessages(const QMailMessageIdList &ids)
{
    // TODO: notify messages that we already have and change the status
    // from read to unread ???

    foreach (const QMailMessageId &id, ids) {
        if (_publishedMessageList.contains(id)) {
            QMailMessageMetaData message(id);
            // Check if message was read
            if (!notifyMessage(message)) {
                _oldMessagesCount++;
                _publishedMessageList.remove(id);
            }
        }
    }
}

void MailStoreObserver::transmitCompleted(const QMailAccountId &accountId)
{
    QList<QObject*> publishedNotifications = Notification::notifications();

    if (publishedNotifications.size()) {
        for (int i = 0; i < publishedNotifications.size(); i++) {
            Notification *publishedNotification = static_cast<Notification*>(publishedNotifications.at(i));
            if (publishedNotification->hintValue("x-nemo.email.sendFailed-accountId").toULongLong() == accountId.toULongLong()) {
                publishedNotification->close();
            }
        }
    }
    qDeleteAll(publishedNotifications);
}

void MailStoreObserver::transmitFailed(const QMailAccountId &accountId)
{
    QList<QObject*> publishedNotifications = Notification::notifications();

    if (publishedNotifications.size()) {
        for (int i = 0; i < publishedNotifications.size(); i++) {
            Notification *publishedNotification = static_cast<Notification*>(publishedNotifications.at(i));
            if (publishedNotification->hintValue("x-nemo.email.sendFailed-accountId").toULongLong() == accountId.toULongLong()) {
                qDeleteAll(publishedNotifications);
                return;
            }
        }
    }

    qDeleteAll(publishedNotifications);

    // Check if there are messages queued to send, transmition failed can be emitted for account testing or by other processes
    // working with the mail store.
    QMailMessageKey outboxFilter(QMailMessageKey::status(QMailMessage::Outbox) & ~QMailMessageKey::status(QMailMessage::Trash));
    QMailMessageKey accountKey(QMailMessageKey::parentAccountId(accountId));
    if (!QMailStore::instance()->countMessages(accountKey & outboxFilter)) {
        return;
    }

    QMailAccount account(accountId);
    QString accountName = account.name();
    QVariant acctId = static_cast<int>(accountId.toULongLong());

    //: Summary of email sending failed notification
    //% "Email sending failed"
    QString summary = qtTrId("qmf-notification_send_failed_summary");
    //: Preview of email sending failed notification
    //% "Failed to send email from account %1"
    QString previewBody = qtTrId("qmf-notification_send_failed_previewBody").arg(accountName);
    //: Body of email sending failed notification
    //% "Account %1"
    QString body = qtTrId("qmf-notification_send_failed_Body").arg(accountName);

    Notification sendFailure;
    sendFailure.setCategory("x-nemo.email.error");
    sendFailure.setHintValue("x-nemo.email.sendFailed-accountId", accountId.toULongLong());
    sendFailure.setPreviewSummary(summary);
    sendFailure.setPreviewBody(previewBody);
    sendFailure.setSummary(summary);
    sendFailure.setBody(body);
    sendFailure.setRemoteAction(::remoteAction("default", "", "openOutbox", QVariantList() << acctId));
    sendFailure.publish();
}

void MailStoreObserver::setNotifyOn()
{
    _appOnScreen = false;
}

void MailStoreObserver::setNotifyOff()
{
    // App is on screen now, close the notification
    if (_publishedItemCount > 0) {
        closeNotifications();
    }
    _appOnScreen = true;
}
