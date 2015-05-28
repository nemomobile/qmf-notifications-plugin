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

#ifndef MAILSTOREOBSERVER_H
#define MAILSTOREOBSERVER_H

// nemonotifications-qt5
#include <notification.h>

// QMF
#include <qmailstore.h>

// Qt
#include <QObject>
#include <QSharedPointer>

struct MessageInfo
{
    QMailMessageId id;
    QString origin;
    QString sender;
    QString subject;
    QDateTime timeStamp;
    QMailAccountId accountId;
};

class MailStoreObserver : public QObject
{
    Q_OBJECT
public:
    explicit MailStoreObserver(QObject *parent = 0);

private slots:
    void actionsCompleted();
    void addMessages(const QMailMessageIdList &ids);
    void removeMessages(const QMailMessageIdList &ids);
    void updateMessages(const QMailMessageIdList &ids);
    void transmitCompleted(const QMailAccountId &accountId);
    void transmitFailed(const QMailAccountId &accountId);
    void setNotifyOn();
    void setNotifyOff();
    void combinedInboxDisplayed();
    void accountInboxDisplayed(int accountId);
    
private:
    typedef QHash<QMailMessageId, QSharedPointer<MessageInfo> > MessageHash;

    bool _publicationChanges;
    bool _appOnScreen;
    QMailStore *_storage;
    MessageHash _publishedMessages;
    QSet<QMailMessageId> _newMessages;

    void reloadNotifications();
    void closeNotifications();
    void closeAccountNotifications(const QMailAccountId &accountId);
    QSharedPointer<MessageInfo> constructMessageInfo(const QMailMessageMetaData &message);
    bool notifyMessage(const QMailMessageMetaData &message);
    void notifyOnly();
    void updateNotifications();
};

#endif // MAILSTOREOBSERVER_H
