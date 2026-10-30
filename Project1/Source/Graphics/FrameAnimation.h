#pragma once

namespace downwell {

struct AnimationClip {
    static constexpr float DefaultFrameDuration = 0.1f;

    int firstFrame = 0;
    int frameCount = 1;
    float frameDuration = DefaultFrameDuration;
    bool loop = true;
};

// 画像や物体の種類に依存せず、時間に合わせて再生するコマを選ぶ。
class FrameAnimation final {
public:
    void SetClip(const AnimationClip& clip);
    void Restart();
    void Update(float deltaTime);

    const AnimationClip& Clip() const { return m_clip; }
    int Frame() const;
    bool Finished() const { return m_finished; }

    void SetPaused(bool paused) { m_paused = paused; }
    bool Paused() const { return m_paused; }

private:
    AnimationClip m_clip{};
    double m_elapsed = 0.0;
    bool m_finished = false;
    bool m_paused = false;
};

}
