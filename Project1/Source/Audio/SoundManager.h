#pragma once

#include "SoundEffect.h"

#include <filesystem>
#include <string>
#include <unordered_map>

namespace downwell {

// BGMと効果音を持ち、DxLibの終了前にまとめて解放する。
class SoundManager final {
public:
    static constexpr float DefaultSEGain = 1.0f;

    SoundManager() = default;
    ~SoundManager();
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    bool LoadBgm(const std::filesystem::path& file);
    bool LoadSE(const std::string& name, const std::filesystem::path& file,
        float gain = DefaultSEGain);
    bool PlayBgm(bool loop = true);
    bool PlaySE(const std::string& name);
    bool LoadSE(SoundEffect effect, const std::filesystem::path& file,
        float gain = DefaultSEGain) {
        return LoadSE(SoundEffectName(effect), file, gain);
    }
    bool PlaySE(SoundEffect effect) {
        return PlaySE(SoundEffectName(effect));
    }
    void StopBgm();
    void StopSE(const std::string& name);
    void StopAll();
    void Clear();

    // 音量は0～255。BGMと効果音を別々に調整する。
    void SetBgmVolume(int volume);
    void SetSEVolume(int volume);
    int BgmVolume() const { return m_bgmVolume; }
    int SEVolume() const { return m_seVolume; }
    bool IsBgmPlaying() const;
    bool IsSEPlaying(const std::string& name) const;

    static constexpr int MinVolume = 0;
    static constexpr int MaxVolume = 255;
    static constexpr int DefaultBgmVolume = 128;
    static constexpr int DefaultSEVolume = 200;

private:
    static int LoadSound(const std::filesystem::path& file, int buffers, int dataType);
    int EffectVolume(float gain) const;

    static constexpr int InvalidHandle = -1;
    static constexpr int BgmBufferCount = 1;
    static constexpr int SEBufferCount = 4;
    static constexpr float MinSEGain = 0.0f;

    struct EffectSound {
        int handle = InvalidHandle;
        float gain = DefaultSEGain;
    };

    int m_bgm = InvalidHandle;
    std::unordered_map<std::string, EffectSound> m_effects;
    int m_bgmVolume = DefaultBgmVolume;
    int m_seVolume = DefaultSEVolume;
    bool m_bgmLoop = true;
};

}
