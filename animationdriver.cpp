// Copyright (C) The Qt Company Ltd.
// SPDX-License-Identifier: BSD-3-Clause
#include "animationdriver.h"

AnimationDriver::AnimationDriver(int msPerStep)
    : m_step(msPerStep)
    , m_elapsed(0)
{
}

void AnimationDriver::advance()
{
    m_elapsed += m_step;
    advanceAnimation();
}

qint64 AnimationDriver::elapsed() const { return m_elapsed; }
