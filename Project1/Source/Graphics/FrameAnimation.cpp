#include "FrameAnimation.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace downwell {
namespace {

constexpr int MinimumFrame = 0;
constexpr int MinimumFrameCount = 1;
// 秒からコマへの変換で生じる、小さな丸め誤差を吸収する。
constexpr double FrameBoundaryTolerance = 1.0e-6;

AnimationClip ValidateClip(AnimationClip clip) {
    clip.firstFrame = std::max(MinimumFrame, clip.firstFrame);
    const auto availableFrames = static_cast<long long>(std::numeric_limits<int>::max())
        - clip.firstFrame + MinimumFrameCount;
    clip.frameCount = static_cast<int>(std::clamp(static_cast<long long>(clip.frameCount),
        static_cast<long long>(MinimumFrameCount), availableFrames));

    if (!std::isfinite(clip.frameDuration) || clip.frameDuration <= 0.0f) {
        clip.frameDuration = AnimationClip::DefaultFrameDuration;
    }

    return clip;
}

}

void FrameAnimation::SetClip(const AnimationClip& clip) {
    m_clip = ValidateClip(clip);
    Restart();
}

void FrameAnimation::Restart() {
    m_elapsed = 0.0;
    m_finished = false;
}

void FrameAnimation::Update(float deltaTime) {
    if (m_paused || m_finished || !std::isfinite(deltaTime) || deltaTime <= 0.0f) return;

    const double frameDuration = m_clip.frameDuration;
    const double clipDuration = frameDuration * m_clip.frameCount;
    const double tolerance = frameDuration * FrameBoundaryTolerance;
    m_elapsed += deltaTime;

    if (m_clip.loop) {
        m_elapsed = std::fmod(m_elapsed, clipDuration);
        if (clipDuration - m_elapsed <= tolerance) m_elapsed = 0.0;
    } else if (m_elapsed + tolerance >= clipDuration) {
        m_elapsed = clipDuration;
        m_finished = true;
    }
}

int FrameAnimation::Frame() const {
    const double lastFrame = m_clip.frameCount - MinimumFrameCount;
    const double elapsedFrames = std::floor(m_elapsed / m_clip.frameDuration + FrameBoundaryTolerance);
    return m_clip.firstFrame + static_cast<int>(std::min(elapsedFrames, lastFrame));
}

}
