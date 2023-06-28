// Copyright (C) The Qt Company Ltd.
// SPDX-License-Identifier: BSD-3-Clause

#include "RenderJobOpenGl.h"

RenderJobOpenGl::RenderJobOpenGl(QObject* parent)
    : QObject(parent)
{
}

RenderJobOpenGl::~RenderJobOpenGl()
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

bool RenderJobOpenGl::init()
{
    m_context = new QOpenGLContext();
    m_offscreenSurface = new QOffscreenSurface();
    m_renderControl = new QQuickRenderControl();

    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    format.setStencilBufferSize(8);
    format.setVersion(3, 2); // Specify OpenGL version
    format.setProfile(QSurfaceFormat::CoreProfile); // Specify profile
    m_context->setFormat(format);

    if (!m_context->create()) {
        qFatal("Unable to init opengl context");
        return false;
    }
    if (!m_context->isValid()) {
        qFatal("Not isValid opengl context");
        return false;
    }

    m_offscreenSurface->setFormat(m_context->format());
    m_offscreenSurface->create();

    // Create and initialize quick window in the main thread
    m_quickWindow = new QQuickWindow(m_renderControl);
    m_quickWindow->setGraphicsApi(QSGRendererInterface::OpenGL);
    if (!m_context->makeCurrent(m_offscreenSurface)) {
        qFatal("Unable to make context current on offscreen surface");
    }
    if (!m_renderControl->initialize()) {
        qFatal("Unable to initialize QQuickRenderControl");
        return false;
    }

    // Create QML engine
    m_qmlEngine = new QQmlEngine();
    if (!m_qmlEngine->incubationController())
        m_qmlEngine->setIncubationController(m_quickWindow->incubationController());
}

bool RenderJobOpenGl::loadQml()
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

void RenderJobOpenGl::start()
{
    loadQml();
    // emit statusChanged(Status::Running);
    createFbo();

    // Render each frame of movie
    m_frames = m_duration / 1000 * m_fps;
    m_animationDriver = new AnimationDriver(1000 / m_fps);
    m_animationDriver->install();
    m_currentFrame = 0;
    // Start the renderer
    renderNext();
}

void RenderJobOpenGl::cleanup()
{
    m_animationDriver->uninstall();
    delete m_animationDriver;
    m_animationDriver = nullptr;

    destroyFbo();
}

void RenderJobOpenGl::createFbo()
{

    if (m_size.isNull() || m_dpr <= 0) {
        qFatal("Invalid size or device pixel ratio");
    }

    // NOT ALLOWED HERE
    // if (!m_context->makeCurrent(m_offscreenSurface)) {
    //     qFatal("Unable to make context current on offscreen surface");
    //     return;
    // }

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

void RenderJobOpenGl::destroyFbo()
{
    delete m_fbo;
    m_fbo = nullptr;
}

void RenderJobOpenGl::saveImage(const QImage& image, const QString& outputFile)
{
    const auto a = QUrl::fromUserInput(outputFile).toLocalFile();
    qInfo() << "Save:" << image.save(a);
}

void RenderJobOpenGl::renderNext()
{
    if (!m_context->makeCurrent(m_offscreenSurface)) {
        qFatal("Unable to make context current on offscreen surface");
        return;
    }

    //  Polish, synchronize and render the next frame (into our fbo).
    m_renderControl->polishItems();
    m_renderControl->beginFrame();
    // If a dedicated render thread is used, the GUI thread should be blocked for the duration of this call.

    m_renderControl->sync();
    m_renderControl->render();
    m_renderControl->endFrame();
    m_context->functions()->glFlush();

    m_currentFrame++;

    QString outputFile(m_outputDirectory + QDir::separator() + m_outputName + "_" + QString::number(m_currentFrame) + "." + m_outputFormat);
    const auto imageFrameUrl = QUrl::fromUserInput(outputFile).toLocalFile();
    auto image = m_fbo->toImage();
    qInfo() << "Save:" << image.save(imageFrameUrl) << "to:" << imageFrameUrl;

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
