TEMPLATE = lib
TARGET = notifications
CONFIG += plugin hide_symbols

QT += core
QT -= gui

CONFIG += link_pkgconfig
LIBS += -lqmfmessageserver5 -lqmfclient5
PKGCONFIG += qmfclient5 qmfmessageserver5 nemotransferengine-qt5 accounts-qt5 nemonotifications-qt5

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
    x-nemo.email.conf \
    rpm/qmf-notifications-plugin.spec

notifications_conf.files = x-nemo.email.conf
notifications_conf.path = /usr/share/lipstick/notificationcategories/
target.path = $$QMF_INSTALL_ROOT/lib/qmf/plugins5/messageserverplugins

INSTALLS += target notifications_conf


