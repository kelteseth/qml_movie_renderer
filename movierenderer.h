// Copyright (C) The Qt Company Ltd.
// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFuture>
#include <QObject>
#include <QOffscreenSurface>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickGraphicsDevice>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickWindow>
#include <QScreen>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QVector>
#include <QtConcurrent>
#include <memory>

#include "RenderJobOpenGl.h"
#include "RenderJobOpenGlThreaded.h"

class MovieRenderer
    : public QObject {
    Q_OBJECT
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    QML_ELEMENT

public:
    explicit MovieRenderer(QObject* parent = 0);

    Q_INVOKABLE void renderMovie(
        const QString& qmlFile,
        const QString& filename,
        const QString& outputDirectory,
        const QString& outputFormat,
        const QSize& size,
        const qreal devicePixelRatio = 1.0,
        const int durationMs = 1000,
        const int fps = 24);

    int progress() const;
    bool event(QEvent* event) override;
    // bool isRunning();

signals:
    void progressChanged(int progress);
    void finished();
    void fileProgressChanged(int fileProgress);
    void startRenderJob();

private slots:
    void setProgress(int progress);
    void futureFinished();

private:
    // Status m_status = Status::NotRunning;
    int m_progress;
    QVector<QFutureWatcher<void>*> m_futures;
    int m_futureCounter;
    int m_fileProgress = 0;
    QThread* m_renderThread = nullptr;
    std::unique_ptr<RenderJobOpenGl> m_renderJobOpenGl;
    std::unique_ptr<RenderJobOpenGlThreaded> m_renderJobOpenGlThreaded;
};
