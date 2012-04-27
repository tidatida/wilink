include(../../../wilink.pri)
include(../../qnetio/qnetio.pri)
include(../../qsound/qsound.pri)
include(../../3rdparty/qdjango/qdjango.pri)
include(../../3rdparty/qxmpp/qxmpp.pri)

TEMPLATE = lib
CONFIG += qt plugin

QT += declarative network sql xml

TARGET = wiLink

# FIXME: this is a hack so that Q_OS_ANDROID is defined
android {
    DEFINES += ANDROID
}
contains(MEEGO_EDITION,harmattan) {
    DEFINES += MEEGO_EDITION_HARMATTAN
    PREFIX = /usr
}

# embedded version
android|symbian|contains(MEEGO_EDITION,harmattan) {
    DEFINES += WILINK_EMBEDDED
}

# Libraries used internal by idle
android|symbian|contains(MEEGO_EDITION,harmattan) {
    SOURCES += idle_stub.cpp
} else:mac {
    LIBS += -framework Carbon
    SOURCES += idle/idle_mac.cpp
} else:unix {
    DEFINES += HAVE_XSS
    SOURCES += idle/idle_x11.cpp
    LIBS += -lXss -lX11
} else:win32 {
    SOURCES += idle/idle_win.cpp
} else {
    SOURCES += idle/idle_stub.cpp
}

SOURCES += \
    accounts.cpp \
    calls.cpp \
    client.cpp \
    console.cpp \
    conversations.cpp \
    declarative.cpp \
    diagnostics.cpp \
    discovery.cpp \
    history.cpp \
    icons.cpp \
    idle/idle.cpp \
    model.cpp \
    news.cpp \
    notifications.cpp \
    phone.cpp \
    phone/sip.cpp \
    photos.cpp \
    places.cpp \
    player.cpp \
    rooms.cpp \
    roster.cpp \
    settings.cpp \
    shares.cpp \
    systeminfo.cpp \
    updater.cpp

HEADERS += \
    accounts.h \
    calls.h \
    client.h \
    console.h \
    conversations.h \
    declarative.h \
    diagnostics.h \
    discovery.h \
    history.h \
    icons.h \
    idle/idle.h \
    model.h \
    news.h \
    notifications.h \
    photos.h \
    phone.h \
    places.h \
    player.h \
    phone/sip.h \
    phone/sip_p.h \
    rooms.h \
    roster.h \
    settings.h \
    shares.h \
    systeminfo.h \
    updater.h

INCLUDEPATH += \
    $$WILINK_INCLUDE_DIR \
    $$QNETIO_INCLUDE_DIR \
    $$QSOUND_INCLUDE_DIR \
    $$QXMPP_INCLUDEPATH \
    ../../3rdparty/qdjango/src/db \
    ../../qxmpp-extra/diagnostics \
    ../../qxmpp-extra/shares

LIBS += \
    -L../../qnetio/src $$QNETIO_LIBS \
    -L../../qsound/src $$QSOUND_LIBS \
    -L../../3rdparty/qxmpp/src $$QXMPP_LIBS \
    -L../../qxmpp-extra -lqxmpp-extra \
    -L../../3rdparty/qdjango/src/db $$QDJANGO_DB_LIBS

# Installation
QMAKE_TARGET_COMPANY="Wifirst"
QMAKE_TARGET_COPYRIGHT="Copyright (c) 2009-2011 Bollore telecom"
android {
} else:mac {
    ICON = ../data/wiLink.icns
    QMAKE_INFO_PLIST = ../data/wiLink.plist
    QMAKE_POST_LINK = sed -i \"\" -e \"s,@VERSION@,$$VERSION,g\" -e \"s,@COPYRIGHT@,$$QMAKE_TARGET_COPYRIGHT,g\" wiLink.app/Contents/Info.plist
} else:symbian {
    vendorinfo = \
        "; Localised Vendor name" \
        "%{\"$$QMAKE_TARGET_COMPANY\"}" \
        " " \
        "; Unique Vendor name" \
        ":\"$$QMAKE_TARGET_COMPANY\"" \
        " "

    mobile_deployment.pkg_prerules += vendorinfo
    DEPLOYMENT += mobile_deployment

    ICON = ../data/scalable/wiLink.svg

    TARGET.CAPABILITY = "NetworkServices ReadUserData WriteUserData UserEnvironment"
} else:unix {
    QT += dbus
    isEmpty(PREFIX) {
        PREFIX=/usr/local
    }
    desktop.path = $$PREFIX/share/applications
    desktop.files = ../data/wiLink.desktop
    icon32.path = $$PREFIX/share/icons/hicolor/32x32/apps
    icon32.files = ../data/32x32/wiLink.png
    icon64.path = $$PREFIX/share/icons/hicolor/64x64/apps
    icon64.files = ../data/64x64/wiLink.png
    pixmap.path = $$PREFIX/share/pixmaps
    pixmap.files = ../data/wiLink.xpm
    scalable.path = $$PREFIX/share/icons/hicolor/scalable/apps
    scalable.files = ../data/scalable/wiLink.svg
    target.path = $$PREFIX/bin
    INSTALLS += desktop icon32 icon64 pixmap scalable target
}