#include "PlayerAttack.h"
#include "Player.h"
#include "Input.h"
#include <Arm/PlayerArm.h>
#include <Enemy.h>

const std::string PlayerAttack::kGroupName_ = "PlayerAttack";

PlayerAttack::PlayerAttack(Player* player,
    const std::array<std::unique_ptr<PlayerArm>, 2>& arms)
    : player_(player)
    , arms_(&arms)
{
    variables_ = GlobalVariables::GetInstance();

    if (!variables_->GroupExists(kGroupName_)) {
        variables_->CreateGroup(kGroupName_);
    }

    variables_->AddItem(kGroupName_, "Combo Protect Duration", kComboProtectDuration_);
}

// =============================================================
//  Init — ゲージ初期化（Player::Init() から呼ぶ）
// =============================================================
void PlayerAttack::Init()
{
    ultGauge_.Init();
    ultimate_.Init();
}

// =============================================================
//  OnHit — PlayerArm の命中コールバックを受け取りゲージに渡す
// =============================================================
void PlayerAttack::OnHit(PlayerUltGauge::HitType type)
{
    ultGauge_.AddGauge(type);
}

// =============================================================
//  UpdateUltimate — Player::Update() から呼ぶ
// =============================================================
void PlayerAttack::UpdateUltimate(Player* player, Enemy* enemy)
{
    ultimate_.Update(player, enemy);
}

// =============================================================
//  ImGui
// =============================================================
void PlayerAttack::ImGui()
{
    ultGauge_.ImGui();
    ultimate_.ImGui();
}

// =============================================================
//  Update
// =============================================================
void PlayerAttack::Update()
{
    ApplyVariables();

    // ゲージ更新
    ultGauge_.Update();

    // --- 必殺技発動入力 ---
    if (Input::GetInstance()->TriggerKey(DIK_R)) {
        if (ultGauge_.IsReady() && !ultimate_.IsActive()) {
            ultGauge_.ConsumeGauge();
            ultimate_.Start(player_, player_->GetEnemy());
        }
    }

    // 必殺技モーション中は通常攻撃入力をすべてブロック
    if (ultimate_.IsActive()) { return; }

    const auto& a = *arms_;

    // --- コンボ保護バッファの更新 ---
    bool isComboWindowOpen = a[kRArm] &&
        a[kRArm]->CanCombo() &&
        a[kRArm]->GetLastAttackType() == PlayerArm::AttackType::kRightPunch;

    if (isComboWindowOpen) {
        comboProtected_ = true;
        comboProtectTimer_ = kComboProtectDuration_;
    }
    else if (comboProtected_) {
        comboProtectTimer_--;
        if (comboProtectTimer_ <= 0) {
            comboProtected_ = false;
            comboProtectTimer_ = 0;
        }
    }

    if (!Input::GetInstance()->TriggerKey(DIK_SPACE)) {
        return;
    }

    bool hitReacting = player_->IsHitReacting();
    if (hitReacting && !comboProtected_) {
        return;
    }
    if (player_->IsDodging()) {
        return;
    }

    // --- ラッシュ優先 ---
    if (!hitReacting && a[kLArm] && a[kLArm]->CanStartRush()) {
        // -------------------------------------------------------
        // 交互パンチを実現するため、左右の腕に半周期分のタイマーオフセットを与える。
        //   右腕 timerOffset = 0               → interval の 0/2 位相
        //   左腕 timerOffset = kRushInterval/2 → interval の 1/2 位相（半周期ずれ）
        // オフセット値は右腕の PlayerArmRush から interval を取得して算出する。
        // -------------------------------------------------------
        const uint32_t rushInterval = a[kRArm] ? a[kRArm]->GetRushInterval() : kDefaultRushInterval_;
        const uint32_t leftArmOffset = rushInterval / kAlternateOffsetDivisor_;

        if (a[kRArm]) { a[kRArm]->StartRush(kRightArmTimerOffset_); }
        if (a[kLArm]) { a[kLArm]->StartRush(leftArmOffset); }
        return;
    }

    // --- 左パンチコンボ ---
    if ((isComboWindowOpen || comboProtected_) &&
        a[kRArm] && a[kRArm]->GetLastAttackType() == PlayerArm::AttackType::kRightPunch) {
        if (a[kLArm] && a[kLArm]->GetBehavior() == PlayerArm::Behavior::kNormal) {
            a[kLArm]->StartAttack(PlayerArm::AttackType::kLeftPunch);
            comboProtected_ = false;
            comboProtectTimer_ = 0;
        }
        return;
    }

    if (hitReacting) {
        return;
    }

    // --- 右パンチ（初撃） ---
    {
        bool anyAttacking = false;
        for (const auto& arm : *arms_) {
            if (arm->GetBehavior() == PlayerArm::Behavior::kAttack ||
                arm->GetBehavior() == PlayerArm::Behavior::kRush) {
                anyAttacking = true;
                break;
            }
        }

        if (!anyAttacking && a[kRArm]) {
            a[kRArm]->StartAttack(PlayerArm::AttackType::kRightPunch);
        }
    }
}

// =============================================================
//  UI表示判定
// =============================================================
bool PlayerAttack::CanRightPunch() const
{
    const auto& a = *arms_;

    if (player_->IsDodging() || player_->IsHitReacting()) {
        return false;
    }

    if (a[kLArm] && a[kLArm]->CanStartRush()) {
        return false;
    }

    bool isComboWindowOpen = a[kRArm] &&
        a[kRArm]->CanCombo() &&
        a[kRArm]->GetLastAttackType() == PlayerArm::AttackType::kRightPunch;
    if (isComboWindowOpen || comboProtected_) {
        return false;
    }

    if (IsRightPunchActive()) {
        return true;
    }

    bool anyAttacking = false;
    for (const auto& arm : *arms_) {
        if (arm->GetBehavior() == PlayerArm::Behavior::kAttack ||
            arm->GetBehavior() == PlayerArm::Behavior::kRush) {
            anyAttacking = true;
            break;
        }
    }
    return !anyAttacking;
}

bool PlayerAttack::CanLeftPunch() const
{
    const auto& a = *arms_;

    if (player_->IsDodging() || player_->IsHitReacting()) {
        if (!comboProtected_) {
            return false;
        }
    }

    if (a[kLArm] && a[kLArm]->CanStartRush()) {
        return false;
    }

    if (IsLeftPunchActive()) {
        return true;
    }

    bool isComboWindowOpen = a[kRArm] &&
        a[kRArm]->CanCombo() &&
        a[kRArm]->GetLastAttackType() == PlayerArm::AttackType::kRightPunch;
    if (isComboWindowOpen || comboProtected_) {
        return true;
    }

    return false;
}

bool PlayerAttack::CanRush() const
{
    const auto& a = *arms_;

    if (player_->IsDodging() || player_->IsHitReacting()) {
        return false;
    }

    if (IsRushActive()) {
        return true;
    }

    if (a[kLArm] && a[kLArm]->CanStartRush()) {
        return true;
    }

    return false;
}

// =============================================================
//  攻撃実行中判定
// =============================================================
bool PlayerAttack::IsRightPunchActive() const
{
    const auto& a = *arms_;
    return a[kRArm] &&
        a[kRArm]->GetBehavior() == PlayerArm::Behavior::kAttack &&
        a[kRArm]->GetCurrentAttackType() == PlayerArm::AttackType::kRightPunch;
}

bool PlayerAttack::IsLeftPunchActive() const
{
    const auto& a = *arms_;
    return a[kLArm] &&
        a[kLArm]->GetBehavior() == PlayerArm::Behavior::kAttack &&
        a[kLArm]->GetCurrentAttackType() == PlayerArm::AttackType::kLeftPunch;
}

bool PlayerAttack::IsRushActive() const
{
    for (const auto& arm : *arms_) {
        if (arm->GetBehavior() == PlayerArm::Behavior::kRush) {
            return true;
        }
    }
    return false;
}

// =============================================================
//  ApplyVariables
// =============================================================
void PlayerAttack::ApplyVariables()
{
    kComboProtectDuration_ = variables_->GetIntValue(kGroupName_, "Combo Protect Duration");
}