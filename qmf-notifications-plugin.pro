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

# translations
TS_FILE = $$OUT_PWD/qmf-notifications.ts
EE_QM = $$OUT_PWD/qmf-notifications_eng_en.qm
ts.commands += lupdate $$PWD -ts $$TS_FILE
ts.CONFIG += no_check_exist
ts.output = $$TS_FILE
ts.input = .
ts_install.files = $$TS_FILE
ts_install.path = /usr/share/translations/source
ts_install.CONFIG += no_check_exist

# should add -markuntranslated "-" when proper translations are in place (or for testing)
engineering_english.commands += lrelease -idbased $$TS_FILE -qm $$EE_QM
engineering_english.CONFIG += no_check_exist
engineering_english.depends = ts
engineering_english.input = $$TS_FILE
engineering_english.output = $$EE_QM
engineering_english_install.path = /usr/share/translations
engineering_english_install.files = $$EE_QM
engineering_english_install.CONFIG += no_check_exist

QMAKE_EXTRA_TARGETS += ts engineering_english
PRE_TARGETDEPS += ts engineering_english

notifications_conf.files = x-nemo.email.conf
notifications_conf.path = /usr/share/lipstick/notificationcategories/
target.path = $$QMF_INSTALL_ROOT/lib/qmf/plugins5/messageserverplugins

INSTALLS += target notifications_conf ts_install engineering_english_install
