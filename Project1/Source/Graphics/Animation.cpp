   #include "Animation.h"

namespace downwell {
namespace {

bool ValidState(AnimationState state) {
    return static_cast<std::size_t>(state) < static_cast<std::size_t>(AnimationState::Count);
}

}

void Animation::SetClip(AnimationState state, const AnimationClip& clip) {
    if (!ValidState(state)) return;

    m_clips[static_cast<std::size_t>(state)] = clip;
    if (state == m_state) Reset(state);
}

void Animation::Play(AnimationState state, bool restart) {
    if (!ValidState(state) || (state == m_state && !restart)) return;

    Reset(state);
}

void Animation::Update(float deltaTime) {
    m_playback.Update(deltaTime);
}

void Animation::Reset(AnimationState state) {
    m_state = ValidState(state) ? state : AnimationState::Idle;
    m_playback.SetClip(m_clips[static_cast<std::size_t>(m_state)]);
}

}
