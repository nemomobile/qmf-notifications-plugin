TEMPLATE = lib
TARGET = notifications
CONFIG += plugin hide_symbols

QT += core
QT -= gui

CONFIG += link_pkgconfig

equals(QT_MAJOR_VERSION, 4) {
    target.path = $$QMF_INSTALL_ROOT/lib/qmf/plugins/messageserverplugins
    LIBS += -lqmfmessageserver -lqmfclient
    PKGCONFIG += qmfclient qmfmessageserver nemotransferengine accounts-qt nemonotifications
}
equals(QT_MAJOR_VERSION, 5) {
    target.path = $$QMF_INSTALL_ROOT/lib/qmf/plugins5/messageserverplugins
    LIBS += -lqmfmessageserver5 -lqmfclient5
    PKGCONFIG += qmfclient5 qmfmessageserver5 nemotransferengine-qt5 accounts-qt5 nemonotifications-qt5
}

SOURCES += \
    actionobserver.cpp \
    accountscache.cpp \
    notificationsplugin.cpp \
    mailstoreobserver.cpp

HEADERS += \
    actionobserver.h \
    accountscache.h \
    notificationsplugin.h \
    mailstoreobserver.h

OTHER_FILES += \
    x-nemo.email.conf

notifications_conf.files = x-nemo.email.conf
notifications_conf.path = /usr/share/lipstick/notificationcategories/


INSTALLS += target notifications_conf


