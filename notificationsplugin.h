#ifndef NOTIFICATIONSPLUGIN_H
#define NOTIFICATIONSPLUGIN_H

#include "actionobserver.h"
#include <QtPlugin>
#include <qmailmessageserverplugin.h>

class NotificationsPlugin : public QMailMessageServerPlugin
{
    Q_OBJECT
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.NotificationsPluginHandlerFactoryInterface")
#endif
public:
    NotificationsPlugin(QObject *parent = 0);
    ~NotificationsPlugin();

    virtual QString key() const;
    virtual void exec();
    virtual NotificationsPlugin* createService();

private:
    ActionObserver* _actionObserver;
};

#endif // NOTIFICATIONSPLUGIN_H
