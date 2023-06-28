// Copyright (C) The Qt Company Ltd.
// SPDX-License-Identifier: BSD-3-Clause

#include "MovieRenderer.h"

MovieRenderer::MovieRenderer(QObject* parent)
    : QObject(parent)
{
}

void MovieRenderer::renderMovie(
    const QString& qmlFile,
    const QString& filename,
    const QString& outputDirectory,
    const QString& outputFormat,
    const QSize& size,
    const qreal devicePixelRatio,
    const int durationMs,
    const int fps)
{
    // if (m_status != Status::NotRunning) {
    //     qWarning() << "Already running, abort!";
    //     return;
    // }

    setProgress(0);
    m_futureCounter = 0;

    bool single_threaded = false;
    if (single_threaded) {
        m_renderJobOpenGl = std::make_unique<RenderJobOpenGl>();
        QObject::connect(m_renderJobOpenGl.get(), &RenderJobOpenGl::progressChanged, this, &MovieRenderer::setProgress);
        m_renderJobOpenGl->m_qmlFile = qmlFile;
        m_renderJobOpenGl->m_size = size;
        m_renderJobOpenGl->m_frames = durationMs / 1000 * fps;
        m_renderJobOpenGl->m_dpr = devicePixelRatio;
        m_renderJobOpenGl->m_duration = durationMs;
        m_renderJobOpenGl->m_fps = fps;
        m_renderJobOpenGl->m_outputName = filename;
        m_renderJobOpenGl->m_outputDirectory = outputDirectory;
        m_renderJobOpenGl->m_outputFormat = outputFormat;

        m_renderJobOpenGl->init();
        m_renderJobOpenGl->start();
    } else {
        m_renderJobOpenGlThreaded = std::make_unique<RenderJobOpenGlThreaded>();
        QObject::connect(m_renderJobOpenGlThreaded.get(), &RenderJobOpenGlThreaded::progressChanged, this, &MovieRenderer::setProgress);
        m_renderJobOpenGlThreaded->m_qmlFile = qmlFile;
        m_renderJobOpenGlThreaded->m_size = size;
        m_renderJobOpenGlThreaded->m_frames = durationMs / 1000 * fps;
        m_renderJobOpenGlThreaded->m_dpr = devicePixelRatio;
        m_renderJobOpenGlThreaded->m_duration = durationMs;
        m_renderJobOpenGlThreaded->m_fps = fps;
        m_renderJobOpenGlThreaded->m_outputName = filename;
        m_renderJobOpenGlThreaded->m_outputDirectory = outputDirectory;
        m_renderJobOpenGlThreaded->m_outputFormat = outputFormat;
        m_renderJobOpenGlThreaded->initRendering();
        m_renderJobOpenGlThreaded->start();
    }
}

int MovieRenderer::progress() const { return m_progress; }

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
    if (m_futureCounter == (m_renderJobOpenGl->m_frames - 1)) {
        qDeleteAll(m_futures);
        m_futures.clear();
        // m_status = Status::NotRunning;
        emit finished();
    }
}

bool MovieRenderer::event(QEvent* event)
{
    if (event->type() == QEvent::UpdateRequest) {
        m_renderJobOpenGl->renderNext();
        return true;
    }

    return QObject::event(event);
}

// bool MovieRenderer::isRunning() { return m_status == Status::Running; }
