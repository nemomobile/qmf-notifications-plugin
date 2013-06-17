#include "notificationsplugin.h"
#include <qmaillog.h>

NotificationsPlugin::NotificationsPlugin(QObject *parent)
    : QMailMessageServerPlugin(parent)
{
}

NotificationsPlugin::~NotificationsPlugin()
{
}

NotificationsPlugin* NotificationsPlugin::createService()
{
    return this;
}

void NotificationsPlugin::exec()
{
    _actionObserver = new ActionObserver(this);
    qMailLog(Messaging) << "Initiating mail notifications plugin";
}

QString NotificationsPlugin::key() const
{
    return "notifications";
}

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
Q_EXPORT_PLUGIN2(notifications, NotificationsPlugin)
#endif
