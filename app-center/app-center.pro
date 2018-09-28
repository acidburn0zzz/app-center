QT       += core gui widgets

TARGET = app-center
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    main.cpp \
    mainwindow.cpp \
    applicationitemwidget.cpp \
    installitemwidget.cpp

HEADERS += \
    mainwindow.h \
    applicationitemwidget.h \
    installitemwidget.h

FORMS += \
    mainwindow.ui \
    applicationitemwidget.ui \
    installitemwidget.ui

app-center.path = $$PREFIX/usr/bin
app-center.files += app-center

scripts.path = $$PREFIX/usr/bin
scripts.files += \
    app-center-cli \
    os-update-manager

desktop.path = $$PREFIX/usr/share/applications
desktop.files += app-center.desktop

INSTALLS += \
    app-center \
    scripts \
    desktop
