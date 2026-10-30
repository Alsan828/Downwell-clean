#include "Player.h"
#include "../Core/InputManager.h"
#include "../Physics/Ground.h"
#include "../Graphics/Render.h"
#include "../Graphics/SpriteSheet.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace downwell {

Player::Player()
    : GameObject(Size, Size) {
    m_previousDrawBounds = GetCollision();
    m_animation.SetClip(AnimationState::Death,
        { 0, DeathSingleFrame, DeathAnimationDuration, false });
}

bool Player::SetAnimationSprite(AnimationState state, std::shared_ptr<const SpriteSheet> sheet,
    const AnimationClip& clip, float scale, bool facesRight) {
    const auto index = static_cast<std::size_t>(state);
    if (index >= m_stateSprites.size() || !sheet || !std::isfinite(scale) || scale <= 0.0f ||
        clip.firstFrame < 0 || clip.frameCount < 1 ||
        clip.firstFrame > sheet->FrameCount() - clip.frameCount) return false;

    m_stateSprites[index] = { std::move(sheet), scale, facesRight };
    m_animation.SetClip(state, clip);
    return true;
}

void Player::BeginDeath() {
    if (m_dying || m_health != MinHealth) return;

    // 最後に表示した位置と物理の補間率を保ち、世界を巻き戻さない。
    m_deathDrawBounds = DrawBounds();
    m_deathInterpolationAlpha = InterpolationAlpha();
    m_deathStartAngle = m_rotationDirection == 0 ? m_drawAngle
        : m_previousDrawAngle + std::remainder(m_drawAngle - m_previousDrawAngle, FullTurn)
            * m_deathInterpolationAlpha;
    const float targetAngle = m_facingRight ? DeathQuarterTurn : -DeathQuarterTurn;
    m_deathAngleTravel = std::remainder(targetAngle - m_deathStartAngle, FullTurn);
    m_dying = true;
    m_deathTime = 0.0;
    m_drawAngle = m_deathStartAngle;
    m_previousDrawAngle = m_drawAngle;
    m_rotationDirection = 0;
    StopHorizontalMotion();
    m_verticalSpeed = 0.0f;
    m_landingHopReady = false;
    m_animation.Play(AnimationState::Death, true);
    m_animation.Frames().SetPaused(false);
}

float Player::UpdateDeath(float deltaTime) {
    ClearFrameEvents();
    if (!std::isfinite(deltaTime) || deltaTime <= 0.0f) return 0.0f;
    if (!m_dying || DeathFinished()) return deltaTime;

    const double remaining = std::max(0.0, static_cast<double>(DeathDuration) - m_deathTime);
    const double consumed = std::min(static_cast<double>(deltaTime), remaining);
    m_deathTime += consumed;
    m_animation.Update(static_cast<float>(consumed));

    // 先に勢いよく倒れ、最後はゆっくり止まる。
    const float progress = static_cast<float>(std::clamp(m_deathTime / DeathAnimationDuration, 0.0, 1.0));
    const float remainingRotation = 1.0f - progress;
    const float easing = 1.0f - remainingRotation * remainingRotation;
    m_drawAngle = progress < 1.0f ? m_deathStartAngle + m_deathAngleTravel * easing
        : (m_facingRight ? DeathQuarterTurn : -DeathQuarterTurn);
    m_previousDrawAngle = m_drawAngle;

    if (m_deathTime + DeathTimeTolerance >= DeathDuration) {
        m_deathTime = DeathDuration;
        m_active = false;
    }

    // 演出をまたいだ分は、呼び出し側のフェードへ引き継ぐ。
    return static_cast<float>(std::max(0.0, static_cast<double>(deltaTime) - consumed));
}

bool Player::DeathFinished() const {
    return m_dying && m_deathTime >= DeathDuration;
}

bool Player::DeathVisible() const {
    if (!Active() || DeathFinished()) return false;
    if (!m_dying || m_deathTime < DeathAnimationDuration) return true;

    const double blinkTime = m_deathTime - DeathAnimationDuration;
    const int phase = static_cast<int>(std::floor((blinkTime + DeathTimeTolerance) / DeathFlashInterval));
    return phase % DeathFlashPhases == 0;
}

void Player::Reset(int width, int height) {
    Reset(width, height, width * 0.5f, Ground::Top(height) - Height() * 0.5f);
    m_grounded = true;
    m_animation.Reset(AnimationState::Idle);
}

void Player::Reset(int width, int height, float x, float y) {
    ClearFrameEvents();
    m_active = true;
    m_dying = false;
    m_deathTime = 0.0;
    m_deathDrawBounds = {};
    m_deathInterpolationAlpha = 1.0f;
    m_deathStartAngle = 0.0f;
    m_deathAngleTravel = 0.0f;
    // 指定した開始位置へ戻し、移動速度やジャンプ中の状態も初期化する。
    m_windowWidth = width;
    m_windowHeight = height;

    SetPosition(x, y);

    StopHorizontalMotion();
    m_moveDirection = 0;
    m_verticalSpeed = 0.0f;
    ClearJumpBrake();
    m_interpolationAlpha = 0.0f;
    m_facingRight = true;
    m_animation.Reset(AnimationState::Fall);
    m_animation.Frames().SetPaused(false);
    m_drawAngle = 0.0f;
    m_rotationDirection = 0;
    // 空中から開始する。着地したかどうかは地形との衝突で決める。
    m_grounded = false;
    m_landed = false;
    m_jumpCut = false;
    m_landingHopReady = false;
    m_softenApex = false;
    m_hurtTime = 0.0f;
    m_knockbackTime = 0.0f;
    m_knockbackDirection = 0;

    ClampPosition();
    m_previousDrawBounds = GetCollision();
    m_previousDrawAngle = m_drawAngle;
}

void Player::Resize(int width, int height, const BlockList& blocks) {
    if (m_dying) {
        m_windowWidth = width;
        m_windowHeight = height;
        return;
    }

    const float groundOffset = static_cast<float>(Ground::Top(height) - Ground::Top(m_windowHeight));
    SetPosition(m_x + (width - m_windowWidth) * 0.5f, m_y + groundOffset);

    m_windowWidth = width;
    m_windowHeight = height;

    ClampPosition();

    // サイズ変更で新しい地形に重なる場合は、安全な開始位置へ戻す。
    for (const auto& block : blocks) {
        if (block->Destroyed()) continue;
        if (Collision().Overlaps(block->Collision())) {
            Reset(width, height);
            break;
        }
    }

    m_grounded = m_verticalSpeed >= 0.0f && HasSupport(blocks);
    if (m_grounded) {
        m_wallJumpDrift = false;
        m_landingHopReady = false;
        m_softenApex = false;
        m_drawAngle = 0.0f;
        m_rotationDirection = 0;
    }

    // 再配置前の位置を描画へ持ち越さない。
    m_interpolationAlpha = 0.0f;
    m_previousDrawBounds = GetCollision();
    m_previousDrawAngle = m_drawAngle;
    UpdateAnimation(0.0f);
}

void Player::ClearFrameEvents() {
    m_events.clear();
    m_spriteEvents.clear();
}

void Player::ClearJumpBrake() {
    // 短いジャンプの減速だけを解除する。ジャンプの種類は変えない。
    m_jumpBrakeAcceleration = 0.0f;
    m_jumpBrakeTime = 0.0f;
}

void Player::StopHorizontalMotion() {
    m_horizontalSpeed = 0.0f;
    m_wallJumpDrift = false;
}

Player::InputAction Player::ReadInput(const InputManager& input, const BlockList& blocks) {
    m_landed = false;
    InputAction action = InputAction::None;

    // 左は-1、右は1。両方を押している場合は0になる。
    const int direction = static_cast<int>(input.Held(Action::Right)) -
        static_cast<int>(input.Held(Action::Left));
    m_moveDirection = direction;

    if (m_grounded && !HasSupport(blocks)) m_grounded = false;

    // 壁から離れる方向を押したときは壁ジャンプを優先する。
    // 押下した瞬間だけ反応し、長押しでは繰り返さない。
    if (m_knockbackTime <= 0.0f && input.Pressed(Action::JumpShoot)) {
        const bool wallJump = CanWallJump(direction, blocks);
        if (m_grounded || wallJump) {
            StartJump(direction, wallJump);
            action = InputAction::Jump;
        } else {
            // 空中で押し直した時だけ連射を開始する。ジャンプの長押しは含めない。
            action = InputAction::Shoot;
        }
    }

    if (!m_jumpCut && input.Released(Action::JumpShoot) && m_verticalSpeed < 0.0f) {
        // キーを離した際の減速を短時間に分け、上昇速度の急変を避ける。
        m_jumpBrakeAcceleration = -m_verticalSpeed * (1.0f - ShortJumpRatio) / JumpCutDuration;
        m_jumpBrakeTime = JumpCutDuration;
        m_jumpCut = true;
    }

    return action;
}

void Player::BeginPhysicsStep(float deltaTime) {
    m_hurtTime = std::max(0.0f, m_hurtTime - deltaTime);
    m_knockbackTime = std::max(0.0f, m_knockbackTime - deltaTime);
    const Rect previous = GetCollision();
    m_previousDrawBounds = previous;
    m_previousDrawAngle = m_drawAngle;
}

void Player::UpdatePhysics(float deltaTime, BlockList& blocks) {
    StepMovement(m_moveDirection, deltaTime, blocks);
}

void Player::UpdateAnimation(float deltaTime) {
    if (m_dying) return;

    AnimationState state = AnimationState::Idle;
    if (Hurt()) {
        state = AnimationState::Hurt;
    } else if (!m_grounded) {
        state = m_verticalSpeed < 0.0f ? AnimationState::Jump : AnimationState::Fall;
    } else if (std::abs(m_horizontalSpeed) > AnimationMoveThreshold) {
        state = AnimationState::Move;
    }

    if (m_horizontalSpeed > AnimationMoveThreshold) m_facingRight = true;
    else if (m_horizontalSpeed < -AnimationMoveThreshold) m_facingRight = false;

    // 状態とフレームだけを更新し、位置や衝突判定は変えない。
    m_animation.Play(state);
    m_animation.Update(deltaTime);
}

void Player::Draw(Render& render) const {
    if (!DeathVisible()) return;

    const int flash = static_cast<int>((HurtDuration - m_hurtTime) / HurtFlashInterval);
    const unsigned int color = !m_dying && Hurt() && flash % 2 == 0 ? 0xff4040 : 0xffffff;
    const float alpha = render.InterpolationAlpha();
    // 一周の境目では短い方向で補間し、着地後は回転を残さない。
    const float angle = m_rotationDirection == 0 ? m_drawAngle :
        m_previousDrawAngle + std::remainder(m_drawAngle - m_previousDrawAngle, FullTurn) * alpha;
    const Rect bounds = m_dying ? m_deathDrawBounds : InterpolatedBounds(m_previousDrawBounds, alpha);
    const auto& sprite = m_stateSprites[static_cast<std::size_t>(m_animation.State())];
    if (sprite.sheet) {
        const float width = sprite.sheet->FrameWidth() * sprite.scale;
        const float height = sprite.sheet->FrameHeight() * sprite.scale;
        const float centerX = (bounds.left + bounds.right) * 0.5f;
        const float centerY = (bounds.top + bounds.bottom) * 0.5f;
        // 足元は判定の下端に合わせ、回転の中心は元の主人公のままにする。
        const float offsetY = bounds.bottom - height * 0.5f - centerY;
        const float drawX = centerX - std::sin(angle) * offsetY;
        const float drawY = centerY + std::cos(angle) * offsetY;
        const Rect imageBounds{ drawX - width * 0.5f, drawY - height * 0.5f,
            drawX + width * 0.5f, drawY + height * 0.5f };
        if (render.DrawFrame(*sprite.sheet, m_animation.Frame(), imageBounds, angle,
            m_facingRight != sprite.facesRight, color)) return;
    }
    if (DrawAnimationFrame(render, bounds, m_animation.Frame(), angle, !m_facingRight, color)) return;
    render.DrawRotatedRectangle(bounds, angle, color);
}

Rect Player::DrawBounds() const {
    return m_dying ? m_deathDrawBounds : InterpolatedBounds(m_previousDrawBounds, InterpolationAlpha());
}

float Player::DrawY() const {
    const Rect bounds = DrawBounds();
    return (bounds.top + bounds.bottom) * 0.5f;
}

float Player::InterpolationAlpha() const {
    if (m_dying) return m_deathInterpolationAlpha;

    return m_interpolationAlpha;
}

void Player::SetInterpolationAlpha(float alpha) {
    if (std::isfinite(alpha)) m_interpolationAlpha = std::clamp(alpha, 0.0f, 1.0f);
}

void Player::StepMovement(int direction, float deltaTime, BlockList& blocks) {
    const float previousX = m_x;
    float nextX = m_x;

    // 通常の反転は低速からやり直す。壁ジャンプ中と被弾中は勢いを残す。
    if (!m_wallJumpDrift && m_knockbackTime <= 0.0f && direction * m_horizontalSpeed < 0.0f) {
        m_horizontalSpeed = 0.0f;
    }

    // 壁ジャンプはキーを離したときや反転でもゆっくり減速し、横への勢いを残す。
    const float targetSpeed = m_knockbackTime > 0.0f ? m_knockbackDirection * KnockbackSpeed : direction * MoveSpeed;
    const float smoothTime = m_wallJumpDrift ? WallJumpSmoothTime :
        (direction == 0 ? StopSmoothTime : MoveSmoothTime);
    const float decay = std::exp(-deltaTime / smoothTime);
    const float speedDifference = m_horizontalSpeed - targetSpeed;
    nextX += targetSpeed * deltaTime + speedDifference * smoothTime * (1.0f - decay);
    m_horizontalSpeed = targetSpeed + speedDifference * decay;

    const float halfWidth = Width() * 0.5f;
    const float halfHeight = Height() * 0.5f;
    // 移動前から移動後までに通過する壁を調べ、手前で止める。
    for (const Block* block : blocks.Nearby(m_y - halfHeight, m_y + halfHeight)) {
        if (block->Destroyed()) continue;
        if (m_y + halfHeight <= block->Top() || m_y - halfHeight >= block->Bottom()) continue;

        if (nextX > previousX && previousX + halfWidth <= block->Left() &&
            nextX + halfWidth > block->Left()) {
            nextX = block->Left() - halfWidth;
            StopHorizontalMotion();
        } else if (nextX < previousX && previousX - halfWidth >= block->Right() &&
            nextX - halfWidth < block->Right()) {
            nextX = block->Right() + halfWidth;
            StopHorizontalMotion();
        }
    }

    SetPosition(nextX, m_y);
    ClampPosition();

    if (m_grounded && !HasSupport(blocks)) m_grounded = false;
    StepVertical(direction, deltaTime, blocks);

    if (m_rotationDirection != 0) {
        m_drawAngle = std::fmod(m_drawAngle + m_rotationDirection * RotationSpeed * deltaTime, FullTurn);
    }
}

void Player::StepVertical(int direction, float deltaTime, BlockList& blocks) {
    if (m_grounded) return;

    const Gravity& gravity = m_verticalSpeed < 0.0f ? m_riseGravity : m_fallGravity;

    if (m_verticalSpeed < 0.0f && m_jumpBrakeTime > 0.0f) {
        const float brakeStep = std::min(deltaTime, m_jumpBrakeTime);

        // 追加の減速で下降速度を作らず、頂点から先は重力だけに任せる。
        m_verticalSpeed = std::min(m_verticalSpeed + m_jumpBrakeAcceleration * brakeStep, 0.0f);
        m_jumpBrakeTime = std::max(m_jumpBrakeTime - brakeStep, 0.0f);
    }

    // 通常ジャンプと踏み付けの頂点だけに適用し、速い落下は元の重力を使う。
    if (m_verticalSpeed > Gravity::ApexSpeedRange) m_softenApex = false;
    gravity.Apply(m_verticalSpeed, deltaTime, m_softenApex);
    const float previousY = m_y;
    float nextY = m_y + m_verticalSpeed * deltaTime;
    const float halfWidth = Width() * 0.5f;
    const float halfHeight = Height() * 0.5f;
    float headContactY = nextY;
    bool hitHead = false;

    if (m_verticalSpeed >= 0.0f) {
        nextY = ResolveLanding(previousY, nextY, blocks);
    } else {
        // 上昇中は頭が当たる面を探し、最も手前の面を優先する。
        for (const Block* block : blocks.Nearby(nextY - halfHeight, previousY + halfHeight)) {
            if (block->Destroyed()) continue;
            if (m_x + halfWidth <= block->Left() || m_x - halfWidth >= block->Right()) continue;

            if (previousY - halfHeight >= block->Bottom() && nextY - halfHeight <= block->Bottom()) {
                headContactY = std::max(headContactY, block->Bottom() + halfHeight);
                hitHead = true;
            }
        }
    }

    if (hitHead) {
        m_softenApex = false;
        bool brokeBlock = false;

        // 最初に触れた面だけを処理する。接ぎ目では両方のブロックに当たる。
        const float headY = headContactY - halfHeight;
        for (Block* block : blocks.Nearby(headY - BlockContactTolerance, headY + BlockContactTolerance)) {
            if (block->Destroyed()) continue;
            if (m_x + halfWidth <= block->Left() || m_x - halfWidth >= block->Right()) continue;
            if (std::abs(block->Bottom() + halfHeight - headContactY) >= BlockContactTolerance) continue;

            block->Hit();
            if (block->Destroyed()) brokeBlock = true;
        }

        // 衝突後の残り時間でも反動を進め、壊れた瞬間から下へ弾く。
        const float contactTime = (headContactY - previousY) / m_verticalSpeed;
        const float remainingTime = std::clamp(deltaTime - contactTime, 0.0f, deltaTime);
        nextY = headContactY;
        m_verticalSpeed = brokeBlock ? BreakRecoilSpeed : 0.0f;

        if (brokeBlock) {
            nextY = ResolveLanding(headContactY, headContactY + m_verticalSpeed * remainingTime, blocks);
        }
    }

    if (m_grounded && m_landingHopReady) {
        // 通常のジャンプの着地に一度だけ反応し、小跳びの着地では繰り返さない。
        m_landingHopReady = false;

        if (direction != 0) {
            m_verticalSpeed = -LandingHopSpeed;
            m_grounded = false;
            m_jumpCut = true;
        }
    }

    SetPosition(m_x, nextY);

    if (m_verticalSpeed >= 0.0f) {
        ClearJumpBrake();
    }
}

float Player::ResolveLanding(float previousY, float nextY, const BlockList& blocks) {
    // 足元がブロックの上面を通過したら、上面に合わせて着地させる。
    const float halfWidth = Width() * 0.5f;
    const float halfHeight = Height() * 0.5f;

    for (const Block* block : blocks.Nearby(previousY + halfHeight, nextY + halfHeight)) {
        if (block->Destroyed()) continue;
        if (m_x + halfWidth <= block->Left() || m_x - halfWidth >= block->Right()) continue;

        if (previousY + halfHeight <= block->Top() && nextY + halfHeight >= block->Top()) {
            nextY = block->Top() - halfHeight;
            if (!m_grounded && m_verticalSpeed >= LandingSoundMinSpeed) {
                m_events.push_back(Event::Land);
                m_spriteEvents.push_back({ SpriteEffectType::Dust, m_x, block->Top() });
            }
            m_verticalSpeed = 0.0f;
            m_grounded = true;
            m_landed = true;
            m_wallJumpDrift = false;
            m_softenApex = false;
            // 着地の小跳びがあっても、回転は最初に足が触れた時点で止める。
            m_drawAngle = 0.0f;
            m_rotationDirection = 0;
        }
    }

    return nextY;
}

bool Player::HasSupport(const BlockList& blocks) const {
    // 足元を支えるブロックがあるか確認する。足場から出ると落下する。
    const float halfWidth = Width() * 0.5f;
    const float bottom = m_y + Height() * 0.5f;

    for (const Block* block : blocks.Nearby(bottom - BlockContactTolerance, bottom + BlockContactTolerance)) {
        if (block->Destroyed()) continue;
        if (m_x + halfWidth > block->Left() && m_x - halfWidth < block->Right() &&
            std::abs(bottom - block->Top()) < BlockContactTolerance) return true;
    }

    return false;
}

void Player::StartJump(int direction, bool wallJump) {
    m_events.push_back(Event::Jump);
    if (wallJump) {
        // 左右の壁に合わせ、砂ぼこりを90度回して外側へ出す。
        const float wallX = m_x - direction * Width() * 0.5f;
        m_spriteEvents.push_back({ SpriteEffectType::Dust, wallX, m_y, direction * WallJumpDustAngle });
    } else {
        m_spriteEvents.push_back({ SpriteEffectType::Dust, m_x, GetCollision().bottom });
    }
    const float speed = wallJump ? WallJumpSpeed : JumpSpeed;
    m_verticalSpeed = -speed;
    ClearJumpBrake();
    m_grounded = false;
    m_jumpCut = false;
    m_landingHopReady = true;
    m_softenApex = true;
    m_rotationDirection = direction;
    m_drawAngle = 0.0f;
    m_previousDrawAngle = 0.0f;
    m_wallJumpDrift = wallJump;

    // 横への押し出しは高さと分けて調整する。
    if (wallJump) m_horizontalSpeed = direction * WallPushSpeed;
}

bool Player::CanShoot() const {
    return !m_dying && !m_grounded && m_knockbackTime <= 0.0f;
}

void Player::ApplyShot(float fallSpeed) {
    m_events.push_back(Event::Shoot);
    // 弾が出た時だけ設定された上下方向の速度に戻す。空弾倉や発射間隔中は変えない。
    m_verticalSpeed = fallSpeed;
    ClearJumpBrake();
    m_jumpCut = true;
    m_softenApex = false;
}

void Player::NotifyReload() {
    m_events.push_back(Event::Reload);
}

bool Player::CanWallJump(int direction, const BlockList& blocks) const {
    if (direction == 0) return false;

    const Rect body = GetCollision();
    for (const Block* block : blocks.Nearby(body.top, body.bottom)) {
        if (block->Destroyed()) continue;
        // 上下面の角に触れただけの場合は、壁として扱わない。
        if (body.bottom <= block->Top() + WallContactTolerance ||
            body.top >= block->Bottom() - WallContactTolerance) continue;

        const float distance = direction > 0 ? body.left - block->Right() : block->Left() - body.right;
        if (std::abs(distance) <= WallContactTolerance) return true;
    }

    return false;
}

void Player::ClampPosition() {
    const float halfSize = Width() * 0.5f;
    if (m_windowWidth < Width()) {
        SetPosition(m_windowWidth * 0.5f, m_y);
        StopHorizontalMotion();
    } else {
        SetPosition(std::clamp(m_x, halfSize, m_windowWidth - halfSize), m_y);

        if ((m_x <= halfSize && m_horizontalSpeed < 0.0f) ||
            (m_x >= m_windowWidth - halfSize && m_horizontalSpeed > 0.0f)) {
            StopHorizontalMotion();
        }
    }
}

}
