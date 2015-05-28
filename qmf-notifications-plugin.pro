TEMPLATE = subdirs

SUBDIRS = src

OTHER_FILES += \
    conf/* \
    rpm/qmf-notifications-plugin.spec

notifications_conf.files = conf/*
notifications_conf.path = /usr/share/lipstick/notificationcategories/

INSTALLS += notifications_conf
