/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Valerio Valerio <valerio.valerio@jollamobile.com>
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
#include <QDebug>

MailStoreObserver::MailStoreObserver(QObject *parent) :
    QObject(parent),
    _newMessagesCount(0),
    _publishedItemCount(0),
    _oldMessagesCount(0),
    _replacesId(0),
    _notification(new Notification(this))
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

    // If something goes wrong (e.g crash), we close any existent notification.
    // In order to keep proper states, notifications framework would need
    // to save a list of ids per notification, and then we could know on
    // start what is published.
    if (publishedNotification()) {
        qDebug() << "Closing notification " << _replacesId;
        closeNotifications();
    }

}

// Close existent notification
void MailStoreObserver::closeNotifications()
{
        _notification->close();
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

// Returns messageInfo object from the list of published messages
QSharedPointer<MessageInfo> MailStoreObserver::messageInfo(const QMailMessageId id)
{
    // Take first
    if (!id.isValid()) {
        foreach (QSharedPointer<MessageInfo> msgInfo, _publishedMessageList)
            return msgInfo;
    } else {
        Q_ASSERT(_publishedMessageList.contains(id));
        return _publishedMessageList.value(id);
    }
}

// Check if this message should be notified
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

void MailStoreObserver::reformatNotification(bool notify, int newCount)
{
    // All messages read remove notification
    if (newCount == 0) {
        closeNotifications();
        return;
        // Only one new message use message sender and subject
    } else if (newCount == 1) {
        Q_ASSERT(_publishedMessageList.size() == 1);
        QSharedPointer<MessageInfo> msgInfo = messageInfo();
        // If is a need message just added, notify the user.
        if (notify) {
            _notification->setPreviewBody(msgInfo.data()->subject);
            _notification->setPreviewSummary(msgInfo.data()->sender);
        } else {
            _notification->setPreviewSummary(QString());
        }
        _notification->setSummary(msgInfo.data()->sender);
        _notification->setBody(msgInfo.data()->subject);
        _notification->setTimestamp(msgInfo.data()->timeStamp);
    } else {
        QString summary = QString("%1 New email(s)").arg(newCount);
        _notification->setSummary(summary);
        if (notify) {
            QString previewSummary = QString("%1 New email(s)").arg(_newMessagesCount);
            _notification->setPreviewSummary(previewSummary);
        } else {
            _notification->setPreviewSummary(QString());
        }
        _notification->setPreviewBody(QString());
        _notification->setBody(QString());

    }
    _notification->setItemCount(newCount);
    _notification->publish();

    //reset count
    if (notify)
        _newMessagesCount = 0;
    else
        _oldMessagesCount = 0;
}

// Checks if there are pubslishedNotifications for this app
bool MailStoreObserver::publishedNotification()
{
    QList<QObject*> publishedNotifications = _notification->notifications();
    Q_ASSERT(publishedNotifications.size() < 2);

    if (publishedNotifications.size() > 0) {
        Notification *publishedNotification = static_cast<Notification*>(publishedNotifications.at(0));
        _replacesId = publishedNotification->replacesId();
        _publishedItemCount = publishedNotification->itemCount();
        _notification->setReplacesId(_replacesId);
        return true;
    } else {
         _publishedItemCount = 0;
        return false;
    }
}

// ################ Slots #####################

void MailStoreObserver::actionsCompleted()
{
    // if there's already a published notification use it
    publishedNotification();

    if (_oldMessagesCount > 0 && _publishedItemCount > 0) {
        Q_ASSERT(_publishedItemCount >= _oldMessagesCount);
        reformatNotification(false, _publishedItemCount -_oldMessagesCount);
    }
    if (_newMessagesCount > 0) {
        reformatNotification(true, _newMessagesCount + _publishedItemCount);
    }
}

void MailStoreObserver::addMessages(const QMailMessageIdList &ids)
{
    foreach (const QMailMessageId &id, ids) {
        QMailMessageMetaData message(id);
        if (notifyMessage(message)) {
            _newMessagesCount++;
            _publishedMessageList.insert(id,QSharedPointer<MessageInfo>(constructMessageInfo(message)));
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
