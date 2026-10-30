#include "SoundManager.h"

#include <DxLib.h>
#include <algorithm>
#include <cmath>

namespace downwell {

SoundManager::~SoundManager() {
    Clear();
}

int SoundManager::LoadSound(const std::filesystem::path& file, int buffers, int dataType) {
    // この音の読み込みだけに設定を適用し、元の設定を必ず戻す。
    const int previousType = GetCreateSoundDataType();
    if (previousType < 0 || SetCreateSoundDataType(dataType) < 0) return InvalidHandle;

    const int handle = LoadSoundMem(file.c_str(), buffers);
    const int restored = SetCreateSoundDataType(previousType);
    if (handle < 0 || restored < 0) {
        if (handle >= 0) DeleteSoundMem(handle);
        return InvalidHandle;
    }
    return handle;
}

bool SoundManager::LoadBgm(const std::filesystem::path& file) {
    // 長い音楽は流しながら読む。
    const int handle = LoadSound(file, BgmBufferCount, DX_SOUNDDATATYPE_FILE);
    if (handle < 0) return false;
    if (ChangeVolumeSoundMem(m_bgmVolume, handle) < 0) {
        DeleteSoundMem(handle);
        return false;
    }

    // 読み込みに成功してから入れ替え、失敗時は今の曲を残す。
    if (m_bgm != InvalidHandle) DeleteSoundMem(m_bgm);
    m_bgm = handle;
    return true;
}

bool SoundManager::LoadSE(const std::string& name, const std::filesystem::path& file,
    float gain) {
    if (name.empty() || !std::isfinite(gain) || gain < MinSEGain) return false;

    // 先に登録先を確保し、確保に失敗しても音声ハンドルを残さない。
    const auto [entry, inserted] = m_effects.try_emplace(name);
    // 短い効果音はメモリに展開し、同じ音の重なりを可能にする。
    const int handle = LoadSound(file, SEBufferCount, DX_SOUNDDATATYPE_MEMNOPRESS);
    if (handle < 0 || ChangeVolumeSoundMem(EffectVolume(gain), handle) < 0) {
        if (handle >= 0) DeleteSoundMem(handle);
        if (inserted) m_effects.erase(entry);
        return false;
    }

    if (entry->second.handle != InvalidHandle) DeleteSoundMem(entry->second.handle);
    entry->second = { handle, gain };
    return true;
}

int SoundManager::EffectVolume(float gain) const {
    // 全体音量に各効果音の倍率を掛け、DxLibの範囲に収める。
    const double volume = std::clamp(
        static_cast<double>(m_seVolume) * gain,
        static_cast<double>(MinVolume), static_cast<double>(MaxVolume));
    return static_cast<int>(std::lround(volume));
}

bool SoundManager::PlayBgm(bool loop) {
    if (m_bgm == InvalidHandle) return false;
    // 同じ曲の再生中は、呼び直しても先頭へ戻さない。
    if (IsBgmPlaying() && m_bgmLoop == loop) return true;

    const int playType = loop ? DX_PLAYTYPE_LOOP : DX_PLAYTYPE_BACK;
    if (PlaySoundMem(m_bgm, playType, TRUE) < 0) return false;
    m_bgmLoop = loop;
    return true;
}

bool SoundManager::PlaySE(const std::string& name) {
    const auto entry = m_effects.find(name);
    if (entry == m_effects.end()) return false;

    const auto& sound = entry->second;
    if (ChangeVolumeSoundMem(EffectVolume(sound.gain), sound.handle) < 0) return false;

    // BACK再生はゲームを止めず、同じ効果音も重ねて鳴らせる。
    return PlaySoundMem(sound.handle, DX_PLAYTYPE_BACK, TRUE) >= 0;
}

void SoundManager::StopBgm() {
    if (m_bgm != InvalidHandle) StopSoundMem(m_bgm);
}

void SoundManager::StopSE(const std::string& name) {
    const auto entry = m_effects.find(name);
    if (entry != m_effects.end()) StopSoundMem(entry->second.handle);
}

void SoundManager::StopAll() {
    StopBgm();
    for (const auto& entry : m_effects) StopSoundMem(entry.second.handle);
}

void SoundManager::Clear() {
    if (m_bgm != InvalidHandle) {
        DeleteSoundMem(m_bgm);
        m_bgm = InvalidHandle;
    }
    for (const auto& entry : m_effects) DeleteSoundMem(entry.second.handle);
    m_effects.clear();
}

void SoundManager::SetBgmVolume(int volume) {
    m_bgmVolume = std::clamp(volume, MinVolume, MaxVolume);
    if (m_bgm != InvalidHandle) ChangeVolumeSoundMem(m_bgmVolume, m_bgm);
}

void SoundManager::SetSEVolume(int volume) {
    m_seVolume = std::clamp(volume, MinVolume, MaxVolume);
    for (const auto& entry : m_effects) {
        const auto& sound = entry.second;
        ChangeVolumeSoundMem(EffectVolume(sound.gain), sound.handle);
    }
}

bool SoundManager::IsBgmPlaying() const {
    return m_bgm != InvalidHandle && CheckSoundMem(m_bgm) == TRUE;
}

bool SoundManager::IsSEPlaying(const std::string& name) const {
    const auto entry = m_effects.find(name);
    return entry != m_effects.end() && CheckSoundMem(entry->second.handle) == TRUE;
}

}
