TEMPLATE = subdirs

SUBDIRS = src

OTHER_FILES += \
    conf/x-nemo.email.conf \
    conf/x-nemo.email.error.conf \
    rpm/qmf-notifications-plugin.spec

notifications_conf.files = conf/x-nemo.email.conf
notifications_conf.path = /usr/share/lipstick/notificationcategories/
notifications_error_conf.files = conf/x-nemo.email.error.conf
notifications_error_conf.path = /usr/share/lipstick/notificationcategories/

INSTALLS += notifications_conf notifications_error_conf
