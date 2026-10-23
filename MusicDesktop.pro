#-------------------------------------------------
# MusicDesktop - Win11 desktop music wallpaper widget
# Qt 5.14.2 + QML, MSVC (C++17), SMTC + WASAPI + WorkerW
#-------------------------------------------------

QT += core gui qml quick widgets quickcontrols2 concurrent network

CONFIG += c++17

TARGET = MusicDesktop
TEMPLATE = app

# 强制 qmake 使用绝对路径，避免相对路径计算错误
QMAKE_PROJECT_DEPTH = 0

DEFINES += QT_DEPRECATED_WARNINGS _SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS

SOURCES += \
    src/main.cpp \
    src/system/WindowHost.cpp \
    src/system/MediaSessionWorker.cpp \
    src/system/AudioCaptureWorker.cpp \
    src/system/FftAnalyzer.cpp \
    src/system/DeviceMonitor.cpp \
    src/core/ConfigStore.cpp \
    src/core/PlayerController.cpp \
    src/core/PlaybackClock.cpp \
    src/core/LyricsEngine.cpp \
    src/core/ThemeEngine.cpp

HEADERS += \
    src/system/WindowHost.h \
    src/system/MediaSessionWorker.h \
    src/system/AudioCaptureWorker.h \
    src/system/FftAnalyzer.h \
    src/system/DeviceMonitor.h \
    src/core/ConfigStore.h \
    src/core/PlayerController.h \
    src/core/PlaybackClock.h \
    src/core/LyricsEngine.h \
    src/core/ThemeEngine.h \
    src/core/SpectrumProvider.h \
    src/core/CoverImageProvider.h \
    src/core/AppActions.h

RESOURCES += resources.qrc

msvc {
    QMAKE_CXXFLAGS += /utf-8 /EHsc /bigobj /await
    QMAKE_CFLAGS += /utf-8
}

# Win32 / WinRT system libraries (lib paths come from vcvars64)
LIBS += user32.lib gdi32.lib ole32.lib oleaut32.lib advapi32.lib runtimeobject.lib

DESTDIR     = $$PWD/bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
RCC_DIR     = $$PWD/build/rcc
