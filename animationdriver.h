// Copyright (C) The Qt Company Ltd.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <QtCore/QAnimationDriver>

class AnimationDriver : public QAnimationDriver {
public:
    AnimationDriver(int msPerStep);

    void advance() override;
    qint64 elapsed() const override;

private:
    int m_step;
    qint64 m_elapsed;
};
