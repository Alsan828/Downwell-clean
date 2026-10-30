#pragma once

namespace downwell {

// ゲーム中の出来事に対応する効果音。
enum class SoundEffect {
    Shoot,
    Jump,
    Land,
    BreakBlock,
    Stomp,
    Hurt,
    Reload
};

constexpr const char* SoundEffectName(SoundEffect effect) {
    switch (effect) {
    case SoundEffect::Shoot: return "shoot";
    case SoundEffect::Jump: return "jump";
    case SoundEffect::Land: return "land";
    case SoundEffect::BreakBlock: return "break_block";
    case SoundEffect::Stomp: return "stomp";
    case SoundEffect::Hurt: return "hurt";
    case SoundEffect::Reload: return "reload";
    }
    return "";
}

}
