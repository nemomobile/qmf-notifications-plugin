TEMPLATE = lib
TARGET = notifications
CONFIG += plugin hide_symbols

QT += core
QT -= gui

CONFIG += link_pkgconfig

equals(QT_MAJOR_VERSION, 4) {
    target.path = $$QMF_INSTALL_ROOT/lib/qmf/plugins/messageserverplugins
    LIBS += -lqmfmessageserver -lqmfclient
    PKGCONFIG += qmfclient qmfmessageserver nemotransferengine accounts-qt
}
equals(QT_MAJOR_VERSION, 5) {
    target.path = $$QMF_INSTALL_ROOT/lib/qmf/plugins5/messageserverplugins
    LIBS += -lqmfmessageserver5 -lqmfclient5
    PKGCONFIG += qmfclient5 qmfmessageserver5 nemotransferengine-qt5 accounts-qt5
}

SOURCES += \
    actionobserver.cpp \
    accountscache.cpp \
    notificationsplugin.cpp

HEADERS += \
    actionobserver.h \
    accountscache.h \
    notificationsplugin.h

INSTALLS += target
