// Copyright (C) The Qt Company Ltd.
// SPDX-License-Identifier: BSD-3-Clause

#include "movierenderer.h"
#include <QCoreApplication>
#include <QEvent>
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

#include <QtConcurrent>

#include "animationdriver.h"

MovieRenderer::MovieRenderer(QObject* parent)
    : QObject(parent)
    , m_context(nullptr)
    , m_offscreenSurface(nullptr)
    , m_renderControl(nullptr)
    , m_quickWindow(nullptr)
    , m_qmlEngine(nullptr)
    , m_qmlComponent(nullptr)
    , m_rootItem(nullptr)
    , m_fbo(nullptr)
    , m_animationDriver(nullptr)
    , m_status(NotRunning)
{
    QSurfaceFormat format;
    // Qt Quick may need a depth and stencil buffer. Always make sure these are
    // available.
    format.setDepthBufferSize(16);
    format.setStencilBufferSize(8);
    format.setVersion(3, 2); // Specify OpenGL version
    format.setProfile(QSurfaceFormat::CoreProfile); // Specify profile

    m_context = new QOpenGLContext;
    m_context->setFormat(format);
    if (!m_context->create()) {
        qFatal("Unable to init opengl context");
    }
    if (!m_context->isValid()) {
        qFatal("Not isValid opengl context");
    }

    m_offscreenSurface = new QOffscreenSurface;
    m_offscreenSurface->setFormat(m_context->format());
    m_offscreenSurface->create();

    m_renderControl = new QQuickRenderControl(this);
    m_quickWindow = new QQuickWindow(m_renderControl);
    m_quickWindow->setGraphicsApi(QSGRendererInterface::OpenGL);
    if (!m_context->makeCurrent(m_offscreenSurface)) {
        qFatal("Unable to make context current on offscreen surface");
    }

    // Initialize the QQuickRenderControl after setting up the context and surface
    if (!m_renderControl->initialize()) {
        qFatal("Unable to initialize QQuickRenderControl");
    }

    QObject::connect(m_quickWindow, &QQuickWindow::sceneGraphInitialized, this, [this]() {
        qInfo() << "sceneGraphInitialized";
    });
    QObject::connect(m_quickWindow, &QQuickWindow::sceneGraphInvalidated, this, [this]() {
        qInfo() << "sceneGraphInvalidated";
    });
    QObject::connect(m_renderControl, &QQuickRenderControl::renderRequested, this, [this]() {
        qInfo() << "renderRequested";
    });
    QObject::connect(m_renderControl, &QQuickRenderControl::sceneChanged, this, [this]() {
        qInfo() << "sceneChanged";
    });

    m_qmlEngine = new QQmlEngine;
    if (!m_qmlEngine->incubationController())
        m_qmlEngine->setIncubationController(m_quickWindow->incubationController());
}

void MovieRenderer::renderMovie(const QString& qmlFile, const QString& filename,
    const QString& outputDirectory,
    const QString& outputFormat, const QSize& size,
    qreal devicePixelRatio, int durationMs,
    int fps)
{
    if (m_status != NotRunning)
        return;

    m_size = size;
    m_dpr = devicePixelRatio;
    setProgress(0);
    setFileProgress(0);
    m_duration = durationMs;
    m_fps = fps;
    m_outputName = filename;
    m_outputDirectory = outputDirectory;
    m_outputFormat = outputFormat;

    if (!loadQML(qmlFile, size))
        return;

    start();
}

MovieRenderer::~MovieRenderer()
{
    m_context->makeCurrent(m_offscreenSurface);
    delete m_renderControl;
    delete m_qmlComponent;
    delete m_quickWindow;
    delete m_qmlEngine;
    delete m_fbo;

    m_context->doneCurrent();

    delete m_offscreenSurface;
    delete m_context;
    delete m_animationDriver;
}

int MovieRenderer::progress() const { return m_progress; }

void MovieRenderer::start()
{
    m_status = Running;
    createFbo();

    // Render each frame of movie
    m_frames = m_duration / 1000 * m_fps;
    m_animationDriver = new AnimationDriver(1000 / m_fps);
    m_animationDriver->install();
    m_currentFrame = 0;
    m_futureCounter = 0;

    // Start the renderer
    renderNext();
}

void MovieRenderer::cleanup()
{
    m_animationDriver->uninstall();
    delete m_animationDriver;
    m_animationDriver = nullptr;

    destroyFbo();
}

void MovieRenderer::createFbo()
{
    if (m_size.isNull() || m_dpr <= 0) {
        qFatal("Invalid size or device pixel ratio");
    }
    QOpenGLContext* ctx = QOpenGLContext::currentContext();

    if (!ctx) {
        qFatal("No context");
        return;
    }

    m_fbo = new QOpenGLFramebufferObject(
        m_size * m_dpr, QOpenGLFramebufferObject::CombinedDepthStencil);

    if (!m_fbo->isValid()) {
        qFatal("invalid m_fbo");
    }

    QQuickRenderTarget renderTarget = QQuickRenderTarget::fromOpenGLTexture(
        m_fbo->texture(), m_fbo->size());

    if (renderTarget.isNull()) {
        qFatal("invalid renderTarget");
    }

    m_quickWindow->setRenderTarget(renderTarget);
}

void MovieRenderer::destroyFbo()
{
    delete m_fbo;
    m_fbo = nullptr;
}

bool MovieRenderer::loadQML(const QString& qmlFile, const QSize& size)
{
    if (m_qmlComponent != nullptr)
        delete m_qmlComponent;
    m_qmlComponent = new QQmlComponent(m_qmlEngine, QUrl(QUrl::fromUserInput(qmlFile)),
        QQmlComponent::PreferSynchronous);

    if (m_qmlComponent->isError()) {
        const QList<QQmlError> errorList = m_qmlComponent->errors();
        for (const QQmlError& error : errorList)
            qWarning() << error.url() << error.line() << error;
        return false;
    }

    QObject* rootObject = m_qmlComponent->create();
    if (m_qmlComponent->isError()) {
        const QList<QQmlError> errorList = m_qmlComponent->errors();
        for (const QQmlError& error : errorList)
            qWarning() << error.url() << error.line() << error;
        return false;
    }

    m_rootItem = qobject_cast<QQuickItem*>(rootObject);
    if (!m_rootItem) {
        qWarning("run: Not a QQuickItem");
        delete rootObject;
        return false;
    }

    // The root item is ready. Associate it with the window.
    m_rootItem->setParentItem(m_quickWindow->contentItem());

    m_rootItem->setWidth(size.width());
    m_rootItem->setHeight(size.height());

    m_quickWindow->setGeometry(0, 0, size.width(), size.height());

    return true;
}

void static saveImage(const QImage& image, const QString& outputFile)
{
    const auto a = QUrl::fromUserInput(outputFile).toLocalFile();
    qInfo() << "Save:" << image.save(a);
}

void MovieRenderer::renderNext()
{
    //  Polish, synchronize and render the next frame (into our fbo).
    m_renderControl->polishItems();
    m_renderControl->beginFrame();
    m_renderControl->sync();
    m_renderControl->render();
    m_renderControl->endFrame();
    m_context->functions()->glFlush();

    m_currentFrame++;

    QString outputFile(m_outputDirectory + QDir::separator() + m_outputName + "_" + QString::number(m_currentFrame) + "." + m_outputFormat);

    QFutureWatcher<void>* watcher = new QFutureWatcher<void>();
    connect(watcher, SIGNAL(finished()), this, SLOT(futureFinished()));
    watcher->setFuture(
        QtConcurrent::run(saveImage, m_fbo->toImage(), outputFile));
    m_futures.append(watcher);

    // advance animation
    setProgress(m_currentFrame / (float)m_frames * 100);
    m_animationDriver->advance();

    if (m_currentFrame < m_frames) {
        // Schedule the next update
        QEvent* updateRequest = new QEvent(QEvent::UpdateRequest);
        QCoreApplication::postEvent(this, updateRequest);
    } else {
        // Finished
        cleanup();
    }
}

void MovieRenderer::setProgress(int progress)
{
    if (m_progress == progress)
        return;
    m_progress = progress;
    emit progressChanged(progress);
}

void MovieRenderer::futureFinished()
{
    m_futureCounter++;
    setFileProgress(m_futureCounter / (float)m_frames * 100);
    if (m_futureCounter == (m_frames - 1)) {
        qDeleteAll(m_futures);
        m_futures.clear();
        m_status = NotRunning;
        emit finished();
    }
}

bool MovieRenderer::event(QEvent* event)
{
    if (event->type() == QEvent::UpdateRequest) {
        renderNext();
        return true;
    }

    return QObject::event(event);
}

bool MovieRenderer::isRunning() { return m_status == Running; }

int MovieRenderer::fileProgress() const { return m_fileProgress; }

void MovieRenderer::setFileProgress(int fileProgress)
{
    if (m_fileProgress == fileProgress)
        return;

    m_fileProgress = fileProgress;
    emit fileProgressChanged(fileProgress);
}
