TEMPLATE = app
TARGET   = lab_chemical_control
CONFIG  += c++11 qt
QT      += core gui widgets

# Source files
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    hardwaremanager.cpp \
    rfidcard.cpp \
    gasmonitor.cpp \
    weather.cpp \
    voc21sensor.cpp \
    alarm.cpp

HEADERS += \
    mainwindow.h \
    hardwaremanager.h \
    rfidcard.h \
    gasmonitor.h \
    weather.h \
    voc21sensor.h \
    alarm.h

# For desktop testing (comment out for ARM cross-compile)
# qmake CONFIG+=desktop
desktop {
    DEFINES += SIMULATE_HARDWARE
    message("Building for DESKTOP (simulated hardware)")
}

# For GEC6818 ARM target (default)
!desktop {
    message("Building for GEC6818 ARM target")
    # If using cross-compiler, set in qmake.conf or pass:
    # qmake -spec linux-arm-gnueabihf-g++
}
