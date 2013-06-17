#include "accountscache.h"

#include <Accounts/Provider>

AccountsCache::AccountsCache(QObject *parent) :
    QObject(parent)
{
    connect(_manager, SIGNAL(accountCreated(Accounts::AccountId)), this, SLOT(accountCreated(Accounts::AccountId)));
    connect(_manager, SIGNAL(accountRemoved(Accounts::AccountId)), this, SLOT(accountRemoved(Accounts::AccountId)));
    connect(_manager, SIGNAL(enabledEvent(Accounts::AccountId)), this, SLOT(enabledEvent(Accounts::AccountId)));
    initCache();
}

void AccountsCache::initCache()
{
    Accounts::AccountIdList accountIDList = _manager->accountListEnabled("e-mail");

    foreach (Accounts::AccountId accountId, accountIDList) {
        Accounts::Account* account = _manager->account(accountId);
        if (account->enabled())
            _accountsList.insert(accountId, account);
    }
}

bool AccountsCache::isEnabledMailAccount(const Accounts::AccountId accountId)
{
    Accounts::Account *account = _manager->account(accountId);
    if (!account)
        return false;

    Accounts::ServiceList emailServices = account->enabledServices();
    // Only enabled email accounts
    if (1 != emailServices.count() || !account->enabled())
        return false;

    return true;
}

// ########## Slots ########################

void AccountsCache::accountCreated(Accounts::AccountId accountId)
{
    if (isEnabledMailAccount(accountId)) {
        Accounts::Account *account = _manager->account(accountId);
        _accountsList.insert(accountId, account);
    }
}

void AccountsCache::accountRemoved(Accounts::AccountId accountId)
{
    if (_accountsList.contains(accountId)) {
        delete _accountsList.take(accountId);
    }
}

void AccountsCache::enabledEvent(Accounts::AccountId accountId)
{
    if (isEnabledMailAccount(accountId)) {
        if (!_accountsList.contains(accountId)) {
            Accounts::Account* account = _manager->account(accountId);
            _accountsList.insert(accountId, account);
        }
    } else if (_accountsList.contains(accountId)) {
        accountRemoved(accountId);
    }

}

// ############### Public functions #####################

AccountInfo AccountsCache::accountInfo(const Accounts::AccountId accountId){
    Q_ASSERT(!_accountsList.contains(accountId));

    AccountInfo accInfo;
    accInfo.name = _accountsList.value(accountId)->displayName();
    Accounts::Provider provider = _manager->provider(_accountsList.value(accountId)->providerName());
    accInfo.providerIcon = QUrl(provider.iconName());
    return accInfo;
}
