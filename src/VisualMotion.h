#pragma once

#include "IsaacTypes.h"
#include <algorithm>

inline bool SamePosition(const Vector& left, const Vector& right) {
    return left.x == right.x && left.y == right.y;
}

class VisualMotion {
public:
    Vector Sample(uint32_t frame, const Vector& position, float maximumStep, float phase = 0.0f) {
        if (!std::isfinite(position.x) || !std::isfinite(position.y)) {
            *this = {};
            return position;
        }

        if (sampled && frame == lastFrame) {

            if (!SamePosition(position, lastPosition)) ambiguous = true;
            return ambiguous ? position : PreviewAt(frame, phase);
        }

        const Vector step{position.x - lastPosition.x, position.y - lastPosition.y};
        const bool continuous = step.x * step.x + step.y * step.y <= maximumStep * maximumStep;
        if (!sampled || frame - lastFrame != 1 || ambiguous || !continuous) {
            *this = {};
            sampled = true;
            lastFrame = changedFrame = frame;
            previous = lastPosition = position;
            return position;
        }

        if (!SamePosition(position, lastPosition)) {
            const uint32_t elapsed = frame - changedFrame;

            if (elapsed <= 2) interval = elapsed;
            previous = lastPosition;
            changedFrame = frame;
        }

        lastFrame = frame;
        lastPosition = position;
        return PreviewAt(frame, phase);
    }

private:
    Vector PreviewAt(uint32_t frame, float phase) const {
        const float elapsed = static_cast<float>(frame - changedFrame) + phase;
        const float blend = std::clamp(elapsed / static_cast<float>(interval), 0.0f, 1.0f);
        return {previous.x + (lastPosition.x - previous.x) * blend,
                previous.y + (lastPosition.y - previous.y) * blend};
    }

    uint32_t lastFrame = 0;
    uint32_t changedFrame = 0;
    uint32_t interval = 1;
    Vector lastPosition;
    Vector previous;
    bool sampled = false;
    bool ambiguous = false;
};
