// Copyright (C) The Qt Company Ltd.
// SPDX-License-Identifier: BSD-3-Clause

#include "RenderJobOpenGlThreaded.h"

RenderJobOpenGlThreaded::~RenderJobOpenGlThreaded()
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

bool RenderJobOpenGlThreaded::initRendering()
{

    // Create and initialize quick window in the main thread
    m_renderControl = new QQuickRenderControl();
    m_quickWindow = new QQuickWindow(m_renderControl);
    m_quickWindow->setGraphicsApi(QSGRendererInterface::OpenGL);
    if (!m_renderControl->initialize()) {
        qFatal("Unable to initialize QQuickRenderControl");
        return false;
    }

    m_format.setDepthBufferSize(16);
    m_format.setStencilBufferSize(8);
    m_format.setVersion(3, 2); // Specify OpenGL version
    m_format.setProfile(QSurfaceFormat::CoreProfile); // Specify profile
    m_offscreenSurface = new QOffscreenSurface();
    m_offscreenSurface->setFormat(m_format);
    m_offscreenSurface->create();

    // Create QML engine
    m_qmlEngine = new QQmlEngine();
    if (!m_qmlEngine->incubationController())
        m_qmlEngine->setIncubationController(m_quickWindow->incubationController());

    loadQml();

    return true;
}

bool RenderJobOpenGlThreaded::loadQml()
{
    if (m_qmlComponent != nullptr)
        delete m_qmlComponent;
    m_qmlComponent = new QQmlComponent(m_qmlEngine, QUrl(QUrl::fromUserInput(m_qmlFile)),
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
    m_rootItem->setWidth(m_size.width());
    m_rootItem->setHeight(m_size.height());

    m_quickWindow->setGeometry(0, 0, m_size.width(), m_size.height());
    return true;
}

void RenderJobOpenGlThreaded::startRendering()
{
    m_context = new QOpenGLContext();
    m_context->setFormat(m_format);

    if (!m_context->create()) {
        qFatal("Unable to init opengl context");
        return;
    }
    if (!m_context->isValid()) {
        qFatal("Not isValid opengl context");
        return;
    }

    auto* thread = this->thread();
    // Prepare and move the render control to the target thread
    m_renderControl->prepareThread(thread);
    // m_quickWindow->moveToThread(thread);
    m_offscreenSurface->moveToThread(thread);
    m_context->moveToThread(thread);
    initFbo();

    // if (QThread::currentThread() != this->thread()) {
    //     qFatal("We are not in the right thread");
    //     // return;
    // }

    // emit statusChanged(Status::Running);

    // Render each frame of movie
    m_frames = m_duration / 1000 * m_fps;
    m_animationDriver = new AnimationDriver(1000 / m_fps);
    m_animationDriver->install();
    m_currentFrame = 0;
    // Start the renderer
    renderNext();
}

void RenderJobOpenGlThreaded::cleanup()
{
    m_animationDriver->uninstall();
    delete m_animationDriver;
    m_animationDriver = nullptr;

    destroyFbo();
}

void RenderJobOpenGlThreaded::initFbo()
{

    if (m_size.isNull() || m_dpr <= 0) {
        qFatal("Invalid size or device pixel ratio");
    }

    if (!m_context->makeCurrent(m_offscreenSurface)) {
        qFatal("Unable to make context current on offscreen surface");
        return;
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

void RenderJobOpenGlThreaded::destroyFbo()
{
    delete m_fbo;
    m_fbo = nullptr;
}

void RenderJobOpenGlThreaded::saveImage(const QImage& image, const QString& outputFile)
{
    const auto a = QUrl::fromUserInput(outputFile).toLocalFile();
    qInfo() << "Save:" << image.save(a);
}

void RenderJobOpenGlThreaded::run()
{
    startRendering();
}
void RenderJobOpenGlThreaded::renderNext()
{
    // Q_ASSERT(QThread::currentThread() == thread());

    if (!m_context->makeCurrent(m_offscreenSurface)) {
        qFatal("Unable to make context current on offscreen surface");
        return;
    }

    // Polishing happens on the gui thread.
    m_renderControl->polishItems();
    m_renderControl->beginFrame();
    // If a dedicated render thread is used, the GUI thread should be blocked for the duration of this call.

    // QMutexLocker lock(&m_mutex);
    //  m_qmlEngine->requestRender();
    m_renderControl->sync();

    // The gui thread can now continue.
    m_cond.wakeOne();
    // lock.unlock();

    m_renderControl->render();
    m_renderControl->endFrame();
    m_context->functions()->glFlush();

    m_currentFrame++;

    QString outputFile(m_outputDirectory + QDir::separator() + m_outputName + "_" + QString::number(m_currentFrame) + "." + m_outputFormat);
    const auto imageFrameUrl = QUrl::fromUserInput(outputFile).toLocalFile();
    auto image = m_fbo->toImage();
    qInfo() << "Save:" << image.save(imageFrameUrl) << "to:" << imageFrameUrl;

    // QFutureWatcher<void>* watcher = new QFutureWatcher<void>();
    // connect(watcher, SIGNAL(finished()), this, SLOT(futureFinished()));
    // watcher->setFuture(
    //     QtConcurrent::run());
    // m_futures.append(watcher);

    // advance animation
    m_animationDriver->advance();
    emit progressChanged((m_currentFrame / m_frames) * 100);

    if (m_currentFrame < m_frames) {
        renderNext();
    } else {
        // Finished
        cleanup();
    }
}
