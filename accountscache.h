#ifndef ACCOUNTSCACHE_H
#define ACCOUNTSCACHE_H

#include <QObject>
#include <QUrl>
#include <QDebug>
#include <Accounts/Manager>
#include <Accounts/Account>
#include <ssoaccountmanager.h>

struct AccountInfo
{
    QString name;
    QUrl providerIcon;
};

class AccountsCache : public QObject
{
    Q_OBJECT
public:
    explicit AccountsCache(QObject *parent = 0);

    AccountInfo accountInfo(const Accounts::AccountId accountId);

private slots:
    void accountCreated(Accounts::AccountId accountId);
    void accountRemoved(Accounts::AccountId accountId);
    void enabledEvent(Accounts::AccountId accountId);
    
private:
    void initCache();
    bool isEnabledMailAccount(const Accounts::AccountId accountId);
    SSOAccountManager _manager;
    QHash<Accounts::AccountId, Accounts::Account*> _accountsList;
};

#endif // ACCOUNTSCACHE_H
