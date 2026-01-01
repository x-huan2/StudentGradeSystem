QT += core gui sql charts printsupport widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# 添加Charts模块
QT += charts

# 添加资源文件
RESOURCES += \
    resources.qrc

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    databasemanager.cpp \
    scoremodel.cpp

HEADERS += \
    mainwindow.h \
    databasemanager.h \
    scoremodel.h

FORMS += \
    mainwindow.ui
