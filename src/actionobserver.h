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

#ifndef ACTIONOBSERVER_H
#define ACTIONOBSERVER_H

#include "accountscache.h"

// nemotransferengine-qt5
#include <transferengineclient.h>

// QMF
#include <qmailserviceaction.h>

// Qt
#include <QObject>
#include <QSharedPointer>

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
    void transmitCompleted(const QMailAccountId &accountId);
    void transmitFailed(const QMailAccountId &accountId);

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

signals:
    void actionsCompleted();
    void transmitCompleted(const QMailAccountId &accountId);
    void transmitFailed(const QMailAccountId &accountId);

private slots:
    void actionsChanged(QList<QSharedPointer<QMailActionInfo> > actions);
    void actionCompleted(quint64 id);
    void emptyActionQueue();

private:
    bool isNotificationAction(QMailServerRequestType requestType);

    QMailActionObserver *_actionObserver;
    QList<quint64> _completedActions;
    QHash<quint64, RunningAction*> _runningActions;
};

#endif // ACTIONOBSERVER_H
