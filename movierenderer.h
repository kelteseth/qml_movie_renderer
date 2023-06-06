// Copyright (C) The Qt Company Ltd.
// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include <QDir>
#include <QFuture>
#include <QObject>
#include <QQmlEngine>
#include <QSize>
#include <QString>

class QOpenGLContext;
class QOpenGLFramebufferObject;
class QOffscreenSurface;
class QQuickRenderControl;
class QQuickWindow;
class QQmlEngine;
class QQmlComponent;
class QQuickItem;
class AnimationDriver;
class MovieRenderer : public QObject {
    Q_OBJECT
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(int fileProgress READ fileProgress NOTIFY fileProgressChanged)
    QML_ELEMENT

public:
    enum Status {
        NotRunning,
        Running
    };

    explicit MovieRenderer(QObject* parent = 0);

    Q_INVOKABLE void renderMovie(const QString& qmlFile, const QString& filename,
        const QString& outputDirectory, const QString& outputFormat,
        const QSize& size, qreal devicePixelRatio = 1.0,
        int durationMs = 1000, int fps = 24);

    ~MovieRenderer();

    int progress() const;

    bool event(QEvent* event) override;
    bool isRunning();

    int fileProgress() const;

public slots:
    void setFileProgress(int fileProgress);

signals:
    void progressChanged(int progress);
    void finished();

    void fileProgressChanged(int fileProgress);

private slots:
    void start();
    void cleanup();

    void createFbo();
    void destroyFbo();
    bool loadQML(const QString& qmlFile, const QSize& size);

    void renderNext();

    void setProgress(int progress);
    void futureFinished();

private:
    QOpenGLContext* m_context;
    QOffscreenSurface* m_offscreenSurface;
    QQuickRenderControl* m_renderControl;
    QQuickWindow* m_quickWindow;
    QQmlEngine* m_qmlEngine;
    QQmlComponent* m_qmlComponent;
    QQuickItem* m_rootItem;
    QOpenGLFramebufferObject* m_fbo;
    qreal m_dpr;
    QSize m_size;
    AnimationDriver* m_animationDriver;

    int m_progress;
    Status m_status;
    int m_duration;
    int m_fps;
    int m_frames;
    int m_currentFrame;
    QString m_outputName;
    QString m_outputFormat;
    QString m_outputDirectory;
    bool m_quickInitialized = false;
    QVector<QFutureWatcher<void>*> m_futures;
    int m_futureCounter;
    int m_fileProgress;
};
