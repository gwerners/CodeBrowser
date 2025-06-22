QT = core

CONFIG += c++20 cmdline
DEFINES += USING_QT

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

FMT += \
    fmt-src/format.cc \
    fmt-src/os.cc

LUA += \
    sol/minilua.c

SOURCES += \
        $${FMT} \
        $${LUA} \
        work/FullTextIndexer.cpp \
        work/Git.cpp \
        work/PipeAdapter.cpp \
        work/Server.cpp \
        work/Task.cpp \
        work/Utils.cpp \
        work/FullTextSearcher.cpp \
        work/main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    work/ConstStrings.h \
    work/FileInfo.h \
    work/FullTextIndexer.h \
    work/Git.h \
    work/LSMessage.h \
    work/PipeAdapter.h \
    work/PipeCommand.h \
    work/SearchFile.h \
    work/SearchInfo.h \
    work/SearchLine.h \
    work/Server.h \
    work/Task.h \
    work/Utils.h \
    work/FullTextSearcher.h \
    work/minilua.h

INCLUDEPATH += crow asio sol env/include /usr/lib/llvm-18/include
LIBS += -L"$$_PRO_FILE_PWD_/env/lib" -L/usr/lib/llvm-18/lib -lclownfish -llucy -lclang
