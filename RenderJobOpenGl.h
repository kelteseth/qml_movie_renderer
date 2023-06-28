#pragma once

#include "animationdriver.h"
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
#include <QThread>
#include <QTimer>
#include <QVector>
#include <QtConcurrent>
#include <memory>

class RenderJobOpenGl : public QObject {
    Q_OBJECT
public:
    explicit RenderJobOpenGl(QObject* parent = 0);
    ~RenderJobOpenGl();
    QSize m_size;
    QString m_outputName;
    QString m_outputFormat;
    QString m_outputDirectory;
    QString m_qmlFile;
    qreal m_dpr = 0;
    int m_fps = 0;
    int m_frames = 0;
    int m_currentFrame = 0;
    int m_duration = 0;
    QThread* renderThread = nullptr;

public:
    bool init();
    void start();
    void renderNext();

signals:
    // void statusChanged(Status status);
    void progressChanged(int progress);

private:
    bool loadQml();
    void cleanup();
    void createFbo();
    void destroyFbo();
    void saveImage(const QImage& image, const QString& outputFile);

private:
    // Must be created from main (gui) thread
    QQuickRenderControl* m_renderControl = nullptr;
    QQuickWindow* m_quickWindow = nullptr;
    // Must be created in the separate thread
    QOpenGLFramebufferObject* m_fbo = nullptr;
    QOpenGLContext* m_context = nullptr;
    QOffscreenSurface* m_offscreenSurface = nullptr;

    QQmlEngine* m_qmlEngine = nullptr;
    QQmlComponent* m_qmlComponent = nullptr;
    QQuickItem* m_rootItem = nullptr;
    AnimationDriver* m_animationDriver = nullptr;
};