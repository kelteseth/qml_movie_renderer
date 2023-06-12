// Copyright (C) The Qt Company Ltd.
// SPDX-License-Identifier: BSD-3-Clause

#include <QGuiApplication>
#include <QObject>
#include <QOpenGLContext>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QUrl>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(QmlOffscreenRendererPlugin)

int main(int argc, char* argv[])
{

    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    // The first subfolder is the libraryName followed by the regular
    // folder strucutre:     LibararyName/Subfolder
    const QUrl url(u"qrc:/QmlOffscreenRenderer/main.qml"_qs);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
