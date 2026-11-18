#pragma once

#include "IsaacTypes.h"

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
            // A shared sprite can be drawn in several places in a single frame.
            if (!SamePosition(position, lastPosition)) ambiguous = true;
            return ambiguous ? position : PreviewAt(phase);
        }

        if (!sampled || frame - lastFrame != 1 || ambiguous) {
            *this = {};
            sampled = true;
            lastFrame = frame;
            lastPreview = lastPosition = position;
            return position;
        }

        const Vector step{position.x - lastPosition.x, position.y - lastPosition.y};
        const bool held = SamePosition(position, lastPosition);
        const bool continuous = step.x * step.x + step.y * step.y <= maximumStep * maximumStep;

        Vector preview = position;
        Vector perPhase;
        // Keep the existing half-step on phases where the game repeats a pose.
        // Additional draws use the time since this phase, even for 60 Hz motion.
        if (held && measuredStep) {
            perPhase = lastVelocity;
            preview.x += perPhase.x;
            preview.y += perPhase.y;
        } else if (!held && continuous) {
            const float scale = lastHeld ? 0.5f : 1.0f;
            perPhase = {step.x * scale, step.y * scale};
        }

        measuredStep = !held && lastHeld && continuous;
        lastVelocity = perPhase;
        lastHeld = held;
        lastFrame = frame;
        lastPosition = position;
        lastPreview = preview;
        return PreviewAt(phase);
    }

private:
    Vector PreviewAt(float phase) const {
        return {lastPreview.x + lastVelocity.x * phase, lastPreview.y + lastVelocity.y * phase};
    }

    uint32_t lastFrame = 0;
    Vector lastPosition;
    Vector lastPreview;
    Vector lastVelocity;
    bool sampled = false;
    bool lastHeld = false;
    bool measuredStep = false;
    bool ambiguous = false;
};
