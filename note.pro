QT += widgets printsupport
INCLUDEPATH += /usr/include/KF6/KSyntaxHighlighting
LIBS += -lKF6SyntaxHighlighting
CONFIG += c++17
TARGET = note
TEMPLATE = app
SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/editor.cpp \
    src/highlighter.cpp \
    src/theme.cpp \
    src/findreplace.cpp
HEADERS += \
    src/mainwindow.h \
    src/editor.h \
    src/highlighter.h \
    src/theme.h \
    src/findreplace.h
RESOURCES += src/resources.qrc
TRANSLATIONS += translations/note_de.ts
DESTDIR = build
OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc
UI_DIR = build/ui
