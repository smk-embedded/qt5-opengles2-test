
TEMPLATE = app
TARGET = qt5-opengles2-test
INCLUDEPATH += .

SOURCES += main.cpp

target.path = /usr/bin
INSTALLS += target

desktop.path = /usr/share/applications
desktop.files = qt5-opengles2-test.desktop
INSTALLS += desktop

