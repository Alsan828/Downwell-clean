#pragma once

#include "FrameAnimation.h"

#include <array>
#include <cstddef>

namespace downwell {

enum class AnimationState {
    Idle,
    Move,
    Jump,
    Fall,
    Hurt,
    Death,
    Count
};

// プレイヤーの状態に合わせて、共通のフレーム再生を切り替える。
class Animation final {
public:
    void SetClip(AnimationState state, const AnimationClip& clip);
    void Play(AnimationState state, bool restart = false);
    void Update(float deltaTime);
    void Reset(AnimationState state = AnimationState::Idle);

    AnimationState State() const { return m_state; }
    int Frame() const { return m_playback.Frame(); }
    bool Finished() const { return m_playback.Finished(); }
    FrameAnimation& Frames() { return m_playback; }
    const FrameAnimation& Frames() const { return m_playback; }

private:
    static constexpr std::size_t StateCount = static_cast<std::size_t>(AnimationState::Count);

    std::array<AnimationClip, StateCount> m_clips{};
    AnimationState m_state = AnimationState::Idle;
    FrameAnimation m_playback;
};

}
