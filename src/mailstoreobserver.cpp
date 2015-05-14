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

QVariantList remoteActionList(const QString &name, const QString &displayName, const QString &method, const QVariantList &arguments = QVariantList())
{
    QVariantList rv;

    rv.append(remoteAction(name, displayName, method, arguments));
    rv.append(remoteAction("app", "", "openCombinedInbox"));

    return rv;
}

}

MailStoreObserver::MailStoreObserver(QObject *parent) :
    QObject(parent),
    _publicationChanges(false),
    _appOnScreen(false),
    _storage(0)
{
    _storage = QMailStore::instance();

    connect(_storage, SIGNAL(messagesAdded(const QMailMessageIdList&)),
            this, SLOT(addMessages(const QMailMessageIdList&)));
    connect(_storage, SIGNAL(messagesUpdated(const QMailMessageIdList&)),
            this, SLOT(updateMessages(const QMailMessageIdList&)));
    connect(_storage, SIGNAL(messagesRemoved(const QMailMessageIdList&)),
            this, SLOT(removeMessages(const QMailMessageIdList&)));

    reloadNotifications();
    updateNotifications();

    QDBusConnection::sessionBus().connect(QString(), "/com/jolla/email/ui", "com.jolla.email.ui", "displayEntered",
                                          this, SLOT(setNotifyOff()));
    QDBusConnection::sessionBus().connect(QString(), "/com/jolla/email/ui", "com.jolla.email.ui", "displayExit",
                                          this, SLOT(setNotifyOn()));
}

void MailStoreObserver::reloadNotifications()
{
    QSet<QMailMessageId> publishedMessageIds;

    // Find the set of messages we've previously published notifications for
    QList<QObject *> existingNotifications(Notification::notifications());
    foreach (QObject *obj, existingNotifications) {
        if (Notification *notification = qobject_cast<Notification *>(obj)) {
            const QString origin(notification->hintValue("x-nemo.email.published-origin").toString());
            if (!origin.isEmpty()) {
                const QString messageIds(notification->hintValue("x-nemo.email.published-message-ids").toString());

                QList<QMailMessageId> ids;
                foreach (const QString &id, messageIds.split(",", QString::SkipEmptyParts)) {
                    ids.append(QMailMessageId(id.toULongLong()));
                }

                if (!ids.isEmpty()) {
                    publishedMessageIds.unite(ids.toSet());
                } else {
                    notification->close();
                }
            } else {
                notification->close();
            }
        }
    }
    qDeleteAll(existingNotifications);

    if (!publishedMessageIds.isEmpty()) {
        QMailAccountIdList enabledAccounts(QMailStore::instance()->queryAccounts(QMailAccountKey::messageType(QMailMessage::Email)
                                                                               & QMailAccountKey::status(QMailAccount::Enabled)));

        foreach (const QMailMessageId &messageId, publishedMessageIds) {
            const QMailMessageMetaData message(messageId);

            // Checks if parent account is still valid
            // accounts can be removed when messageServer is not running.
            if (enabledAccounts.contains(message.parentAccountId())) {
                if (notifyMessage(message)) {
                    _publishedMessages.insert(messageId, constructMessageInfo(message));
                }
            }
        }
    }
}

// Close existent notification
void MailStoreObserver::closeNotifications()
{
    QList<QObject *> existingNotifications(Notification::notifications());
    foreach (QObject *obj, existingNotifications) {
        if (Notification *notification = qobject_cast<Notification *>(obj)) {
            notification->close();

            const QString messageIds(notification->hintValue("x-nemo.email.published-message-ids").toString());
            if (!messageIds.isEmpty()) {
                // We're no longer publishing these messages
                foreach (const QString &id, messageIds.split(",", QString::SkipEmptyParts)) {
                    _publishedMessages.remove(QMailMessageId(id.toULongLong()));
                }
            }
        }
    }
    qDeleteAll(existingNotifications);
}

// Contructs messageInfo object from a email message
QSharedPointer<MessageInfo> MailStoreObserver::constructMessageInfo(const QMailMessageMetaData &message)
{
    MessageInfo* messageInfo = new MessageInfo();
    messageInfo->id = message.id();
    messageInfo->origin = message.from().address().toLower();
    messageInfo->sender = message.from().name();
    messageInfo->subject = message.subject();
    messageInfo->timeStamp = message.date().toUTC();
    messageInfo->accountId = message.parentAccountId();

    return QSharedPointer<MessageInfo>(messageInfo);
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
    Notification notification;
    notification.setCategory("x-nemo.email.summary");
    notification.publish();
}

void MailStoreObserver::updateNotifications()
{
    QHash<QString, QList<QMailMessageId> > senderMessages;

    // Group the set of publishable messages by sender
    MessageHash::const_iterator it = _publishedMessages.constBegin(), end = _publishedMessages.constEnd();
    for ( ; it != end; ++it) {
        senderMessages[it.value()->origin].append(it.key());
    }

    QHash<QString, uint> existingSenderNotificationIds;

    // Remove any existing notifications whose sender no longer has published messages
    QList<QObject *> existingNotifications(Notification::notifications());
    foreach (QObject *obj, existingNotifications) {
        if (Notification *notification = qobject_cast<Notification *>(obj)) {
            const QString origin(notification->hintValue("x-nemo.email.published-origin").toString());
            if (origin.isEmpty() || !senderMessages.contains(origin)) {
                notification->close();
            } else {
                existingSenderNotificationIds.insert(origin, notification->replacesId());
            }
        }
    }
    qDeleteAll(existingNotifications);

    // Publish a notification for each sender who has qualifying messages
    QHash<QString, QList<QMailMessageId> >::const_iterator sit = senderMessages.constBegin(), send = senderMessages.constEnd();
    for ( ; sit != send; ++sit) {
        const QString &origin(sit.key());
        const QList<QMailMessageId> &messageIds(sit.value());

        QStringList messageIdList;
        foreach (const QMailMessageId &id, messageIds) {
            messageIdList.append(QString::number(id.toULongLong()));
        }

        Notification notification;

        //: Name of notification group for mail notifications
        //% "Email"
        notification.setAppName(qtTrId("qmf-notification_group"));
        notification.setCategory("x-nemo.email");
        notification.setHintValue("x-nemo.email.published-origin", origin);
        notification.setHintValue("x-nemo.email.published-message-ids", messageIdList.join(","));

        QHash<QString, uint>::iterator it(existingSenderNotificationIds.find(origin));
        if (it != existingSenderNotificationIds.end()) {
            // Replace the existing notification for this sender
            notification.setReplacesId(it.value());
        }

        const int senderCount(messageIds.count());
        if (senderCount == 1) {
            // Only a single message from this address
            const MessageInfo *message(_publishedMessages[messageIds.first()].data());

            notification.setSummary(message->sender.isEmpty() ? message->origin : message->sender);
            notification.setBody(message->subject);
            notification.setTimestamp(message->timeStamp);
            notification.setItemCount(1);

            const QVariant msgId(static_cast<int>(messageIds.first().toULongLong()));
            notification.setRemoteActions(::remoteActionList("default", "", "openMessage", QVariantList() << msgId));
        } else {
            QString senderName;
            QDateTime timestamp;
            QSet<QMailAccountId> accountIds;

            foreach (const QMailMessageId &id, messageIds) {
                const MessageInfo *message(_publishedMessages[id].data());

                // Find the name of this sender
                if (senderName.isEmpty() && !message->sender.isEmpty()) {
                    senderName = message->sender;
                }

                timestamp = qMax(timestamp, message->timeStamp);

                // Find the accounts to which we have received messages from this sender
                accountIds.insert(_publishedMessages[id]->accountId);
            }

            if (senderName.isEmpty()) {
                senderName = origin;
            }

            //: Summary of new email(s) notification, %1 is the sender name/address
            //% "%n new email(s) from %1"
            QString summary = qtTrId("qmf-notification_new_emails_notification_text", senderCount);
            notification.setSummary(summary.arg(senderName));

            notification.setTimestamp(timestamp);
            notification.setItemCount(senderCount);

            if (accountIds.count() == 1) {
                const QVariant acctId(static_cast<int>((*accountIds.constBegin()).toULongLong()));
                notification.setRemoteActions(::remoteActionList("default", "", "openInbox", QVariantList() << acctId));
            } else {
                notification.setRemoteActions(::remoteActionList("default", "", "openCombinedInbox"));
            }
        }

        notification.publish();
    }
}

// ################ Slots #####################

void MailStoreObserver::actionsCompleted()
{
    if (_publicationChanges) {
        _publicationChanges = false;

        updateNotifications();

        if (!_newMessages.isEmpty()) {
            // Notify the user of new messages
            if (_appOnScreen) {
                notifyOnly();
            } else {
                Notification summaryNotification;
                summaryNotification.setCategory("x-nemo.email.summary");

                if (_newMessages.count() == 1) {
                    const QMailMessageId messageId(*_newMessages.begin());
                    const MessageInfo *message(_publishedMessages[messageId].data());

                    summaryNotification.setPreviewSummary(message->sender.isEmpty() ? message->origin : message->sender);
                    summaryNotification.setPreviewBody(message->subject);

                    const QVariant varId(static_cast<int>(messageId.toULongLong()));
                    summaryNotification.setRemoteAction(::remoteAction("default", "", "openMessage", QVariantList() << varId));
                } else {
                    //: Summary of new email(s) notification
                    //% "You have %n new email(s)"
                    summaryNotification.setPreviewSummary(qtTrId("qmf-notification_new_email_banner_notification", _newMessages.count()));

                    // Find if these messages are all for the same account
                    QMailAccountId firstAccountId;
                    foreach (const QMailMessageId &messageId, _newMessages) {
                        const MessageInfo *message(_publishedMessages[messageId].data());
                        if (!firstAccountId.isValid()) {
                            firstAccountId = message->accountId;
                        } else if (message->accountId != firstAccountId) {
                            firstAccountId = QMailAccountId();
                            break;
                        }
                    }

                    if (firstAccountId.isValid()) {
                        // Show the inbox for this account
                        const QVariant varId(static_cast<int>(firstAccountId.toULongLong()));
                        summaryNotification.setRemoteAction(::remoteAction("default", "", "openInbox", QVariantList() << varId));
                    } else {
                        // Multiple accounts - show the combined inbox
                        summaryNotification.setRemoteAction(::remoteAction("default", "", "openCombinedInbox"));
                    }
                }

                summaryNotification.publish();
            }

            _newMessages.clear();
        }
    }
}

void MailStoreObserver::addMessages(const QMailMessageIdList &ids)
{
    foreach (const QMailMessageId &id, ids) {
        const QMailMessageMetaData message(id);

        // Workaround for plugin that try to add same message twice
        if (notifyMessage(message) && !_publishedMessages.contains(id)) {
            _publishedMessages.insert(id, constructMessageInfo(message));
            _newMessages.insert(id);
            _publicationChanges = true;
        }
    }
}

void MailStoreObserver::removeMessages(const QMailMessageIdList &ids)
{
    foreach (const QMailMessageId &id, ids) {
        if (_publishedMessages.contains(id)) {
            _publishedMessages.remove(id);
            _newMessages.remove(id);
            _publicationChanges = true;
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
        if (_publishedMessages.contains(id)) {
            const QMailMessageMetaData message(id);
            // Check if message was read
            if (!notifyMessage(message)) {
                _publishedMessages.remove(id);
                _newMessages.remove(id);
                _publicationChanges = true;
            }
        }
    }
}

void MailStoreObserver::transmitCompleted(const QMailAccountId &accountId)
{
    const QVariant acctId(accountId.toULongLong());

    // If there is an existing failure for this notification, remove it
    QList<QObject *> existingNotifications(Notification::notifications());
    foreach (QObject *obj, existingNotifications) {
        if (Notification *notification = qobject_cast<Notification *>(obj)) {
            if (notification->hintValue("x-nemo.email.sendFailed-accountId") == accountId) {
                notification->close();
                break;
            }
        }
    }
    qDeleteAll(existingNotifications);
}

void MailStoreObserver::transmitFailed(const QMailAccountId &accountId)
{
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
    //: Name of notification group for error notifications
    //% "Warnings"
    QString appName = qtTrId("qmf-notification_error_group");

    Notification sendFailure;
    sendFailure.setAppName(appName);
    sendFailure.setCategory("x-nemo.email.error");
    sendFailure.setHintValue("x-nemo.email.sendFailed-accountId", accountId.toULongLong());
    sendFailure.setPreviewSummary(summary);
    sendFailure.setPreviewBody(previewBody);
    sendFailure.setSummary(summary);
    sendFailure.setBody(body);
    sendFailure.setRemoteAction(::remoteAction("default", "", "openOutbox", QVariantList() << acctId));

    // If there is an existing failure for this notification, replace it
    QList<QObject *> existingNotifications(Notification::notifications());
    foreach (QObject *obj, existingNotifications) {
        if (Notification *notification = qobject_cast<Notification *>(obj)) {
            if (notification->hintValue("x-nemo.email.sendFailed-accountId") == accountId) {
                sendFailure.setReplacesId(notification->replacesId());
                break;
            }
        }
    }
    qDeleteAll(existingNotifications);

    sendFailure.publish();
}

void MailStoreObserver::setNotifyOn()
{
    _appOnScreen = false;
}

void MailStoreObserver::setNotifyOff()
{
    closeNotifications();
    _appOnScreen = true;
}
