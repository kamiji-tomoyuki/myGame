#include "PlayerUltimate.h"
#include "Player.h"
#include "FollowCamera.h"
#include <PlayerArm.h>
#include <Enemy.h>
#include "myMath.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>

using namespace Engine;
const std::string PlayerUltimate::kGroupName_ = "PlayerUltimate";

PlayerUltimate::PlayerUltimate()
{
    variables_ = GlobalVariables::GetInstance();
}

// =============================================================
//  Init
// =============================================================
void PlayerUltimate::Init()
{
    if (!variables_->GroupExists(kGroupName_)) {
        variables_->CreateGroup(kGroupName_);
    }
    variables_->AddItem(kGroupName_, "Rise Duration", kRiseDuration_);
    variables_->AddItem(kGroupName_, "Charge Duration", kChargeDuration_);
    variables_->AddItem(kGroupName_, "Dive Duration", kDiveDuration_);
    variables_->AddItem(kGroupName_, "Impact Duration", kImpactDuration_);
    variables_->AddItem(kGroupName_, "Recover Duration", kRecoverDuration_);
    variables_->AddItem(kGroupName_, "Rise Height", kRiseHeight_);
    variables_->AddItem(kGroupName_, "Area Radius", kAreaRadius_);
    variables_->AddItem(kGroupName_, "Area Damage", kAreaDamage_);

    ApplyVariables();
    phase_ = Phase::kIdle;
    timer_ = 0;
    hasHit_ = false;
}

// =============================================================
//  Start
// =============================================================
void PlayerUltimate::Start(Player* player, Enemy* enemy)
{
    if (phase_ != Phase::kIdle) { return; }
    ApplyVariables();

    enemy_ = enemy;   // ★ 毎フレーム追従するために保持

    startPos_ = player->GetCenterPosition();
    // 通常の立ち位置（0.0f）を基準にする
    const float kGroundY = 0.0f;
    startPos_.y = kGroundY; 

    peakPos_ = startPos_;
    peakPos_.y += kRiseHeight_;

    // targetPos_ は Dive 開始時に再計算するので暫定値でよい
    RecalcTarget(player);

    // 腕の初期ローカル姿勢を保存
    SaveArmPose(player);

    phase_ = Phase::kRise;
    timer_ = 0;
    hasHit_ = false;
}

// =============================================================
//  Update
// =============================================================
void PlayerUltimate::Update(Player* player, Enemy* enemy)
{
    if (phase_ == Phase::kIdle) { return; }
    ApplyVariables();

    // 保持している enemy_ を最新ポインタで上書き（念のため）
    if (enemy) { enemy_ = enemy; }

    timer_++;

    // ★ 衝撃発生前（Dive終了まで）のみターゲット位置を更新して追従する
    if (phase_ == Phase::kRise || phase_ == Phase::kCharge || phase_ == Phase::kDive) {
        RecalcTarget(player);
    }
    
    FaceTarget(player);

    switch (phase_) {
    case Phase::kRise:    UpdateRise(player);          break;
    case Phase::kCharge:  UpdateCharge(player);        break;
    case Phase::kDive:    UpdateDive(player);          break;
    case Phase::kImpact:  UpdateImpact(player);        break;
    case Phase::kRecover: UpdateRecover(player);       break;
    default: break;
    }
}

// =============================================================
//  ターゲット方向の毎フレーム再計算
// =============================================================
void PlayerUltimate::RecalcTarget(Player* player)
{
    Vector3 origin = player->GetCenterPosition();

    if (enemy_) {
        Vector3 enemyPos = enemy_->GetCenterPosition();
        float dx = enemyPos.x - origin.x;
        float dz = enemyPos.z - origin.z;
        float len = std::sqrtf(dx * dx + dz * dz);
        if (len > 0.0001f) {
            targetRotY_ = std::atan2f(dx, dz);
        }
        // Dive 着地点も敵位置に追従
        targetPos_ = enemyPos;
        targetPos_.y = startPos_.y;
    }
    else {
        // 敵がいない場合はプレイヤーの向きを維持
        float ry = player->GetCenterRotation().y;
        targetRotY_ = ry;
        Vector3 forward = { std::sinf(ry), 0.0f, std::cosf(ry) };
        targetPos_ = startPos_ + forward * 6.0f;
        targetPos_.y = startPos_.y;
    }
}

// =============================================================
//  向き制御（プレイヤー本体 ＋ 追従カメラ）
// =============================================================
void PlayerUltimate::FaceTarget(Player* player)
{
    Vector3 rot = player->GetCenterRotation();
    rot.y = targetRotY_;
    player->SetRotation(rot);

    if (FollowCamera* cam = player->GetFollowCamera()) {
        cam->SetStableAngleY(targetRotY_);
    }
}

// =============================================================
//  腕姿勢の保存・復帰（ローカル空間で統一）
// =============================================================
void PlayerUltimate::SaveArmPose(Player* player)
{
    const auto& arms = player->GetArms();
    for (int i = 0; i < 2; ++i) {
        if (arms[i]) {
            savedArmRot_[i] = arms[i]->GetLocalRotation();
            savedArmPos_[i] = arms[i]->GetLocalTranslation();
        }
    }
}

void PlayerUltimate::RestoreArmPose(Player* player)
{
    const auto& arms = player->GetArms();
    for (int i = 0; i < 2; ++i) {
        if (arms[i]) {
            arms[i]->SetRotation(savedArmRot_[i]);
            arms[i]->SetTranslation(savedArmPos_[i]);
        }
    }
}

// =============================================================
//  フェーズ別更新
// =============================================================

// --- 飛び上がり ---
void PlayerUltimate::UpdateRise(Player* player)
{
    float t = std::clamp(static_cast<float>(timer_) / kRiseDuration_, 0.0f, 1.0f);
    float eased = 1.0f - (1.0f - t) * (1.0f - t);  // ease-out

    Vector3 pos = startPos_;
    pos.y = startPos_.y + (peakPos_.y - startPos_.y) * eased;
    player->SetTranslation(pos);

    // 腕：位置はそのまま（回転なし）
    const auto& arms = player->GetArms();
    if (arms[0]) { arms[0]->SetTranslation(savedArmPos_[0]); }
    if (arms[1]) { arms[1]->SetTranslation(savedArmPos_[1]); }

    if (timer_ >= kRiseDuration_) {
        player->SetTranslation(peakPos_);
        AdvancePhase(player);
    }
}

// --- 溜め：右腕をローカル後方（-Z）へ引く ---
void PlayerUltimate::UpdateCharge(Player* player)
{
    player->SetTranslation(peakPos_);

    float t = std::clamp(static_cast<float>(timer_) / kChargeDuration_, 0.0f, 1.0f);
    float eased = t * t * (3.0f - 2.0f * t);  // ease-in-out

    const auto& arms = player->GetArms();

    // 右腕（index 0）：後方（-Z）へ引く
    if (arms[0]) {
        Vector3 p = savedArmPos_[0];
        p.z -= eased * kChargePullBack_;   // ローカル -Z = 体の後方
        arms[0]->SetTranslation(p);
        arms[0]->SetRotation(savedArmRot_[0]);   // 回転は変えない
    }
    // 左腕（index 1）：位置変化なし（構えは自然体）
    if (arms[1]) {
        arms[1]->SetTranslation(savedArmPos_[1]);
        arms[1]->SetRotation(savedArmRot_[1]);
    }

    if (timer_ >= kChargeDuration_) {
        AdvancePhase(player);
    }
}

// --- 急降下：右腕を前方（+Z）へ突き出しながら降下 ---
void PlayerUltimate::UpdateDive(Player* player)
{
    float t = std::clamp(static_cast<float>(timer_) / kDiveDuration_, 0.0f, 1.0f);
    float eased = t * t;  // ease-in（加速しながら落下）

    // プレイヤー位置を peakPos_ → targetPos_ へ移動
    Vector3 pos;
    pos.x = peakPos_.x + (targetPos_.x - peakPos_.x) * eased;
    pos.y = peakPos_.y + (targetPos_.y - peakPos_.y) * eased;
    pos.z = peakPos_.z + (targetPos_.z - peakPos_.z) * eased;
    player->SetTranslation(pos);

    const auto& arms = player->GetArms();

    // 右腕：後方引き位置（-kChargePullBack_）→ 前方突き出し（+kDivePunchReach_）へ補間
    if (arms[0]) {
        Vector3 p = savedArmPos_[0];
        float fromZ = -kChargePullBack_;      // Charge 終了時の Z オフセット
        float toZ = kDivePunchReach_;       // Dive 終了時の Z オフセット（前方）
        p.z += fromZ + (toZ - fromZ) * eased;
        arms[0]->SetTranslation(p);
        arms[0]->SetRotation(savedArmRot_[0]);
    }
    // 左腕：位置変化なし
    if (arms[1]) {
        arms[1]->SetTranslation(savedArmPos_[1]);
        arms[1]->SetRotation(savedArmRot_[1]);
    }

    if (timer_ >= kDiveDuration_) {
        player->SetTranslation(targetPos_);
        AdvancePhase(player);
    }
}

// --- 着地パンチ＆範囲攻撃 ---
void PlayerUltimate::UpdateImpact(Player* player)
{
    if (!hasHit_) {
        ApplyAreaDamage(player);
        hasHit_ = true;
    }

    // 右腕：前方突き出しポーズで固定
    const auto& arms = player->GetArms();
    if (arms[0]) {
        Vector3 p = savedArmPos_[0];
        p.z += kDivePunchReach_;
        arms[0]->SetTranslation(p);
        arms[0]->SetRotation(savedArmRot_[0]);
    }
    if (arms[1]) {
        arms[1]->SetTranslation(savedArmPos_[1]);
        arms[1]->SetRotation(savedArmRot_[1]);
    }

    if (timer_ >= kImpactDuration_) {
        AdvancePhase(player);
    }
}

// --- 硬直回復：腕を初期位置へ戻す ---
void PlayerUltimate::UpdateRecover(Player* player)
{
    float t = std::clamp(static_cast<float>(timer_) / kRecoverDuration_, 0.0f, 1.0f);

    // 座標の固定（Y軸のずれ防止）
    player->SetTranslation(targetPos_);

    const auto& arms = player->GetArms();

    // Impact の突き出し位置 → 初期位置 へ線形補間
    if (arms[0]) {
        Vector3 p;
        float punchZ = savedArmPos_[0].z + kDivePunchReach_;
        p.x = savedArmPos_[0].x;
        p.y = savedArmPos_[0].y;
        p.z = punchZ * (1.0f - t) + savedArmPos_[0].z * t;
        arms[0]->SetTranslation(p);
        arms[0]->SetRotation(savedArmRot_[0]);
    }
    if (arms[1]) {
        arms[1]->SetTranslation(savedArmPos_[1]);
        arms[1]->SetRotation(savedArmRot_[1]);
    }

    if (timer_ >= kRecoverDuration_) {
        RestoreArmPose(player);
        phase_ = Phase::kIdle;
        timer_ = 0;
        enemy_ = nullptr;
    }
}

// =============================================================
//  範囲ダメージ
// =============================================================
void PlayerUltimate::ApplyAreaDamage(Player* player)
{
    if (!enemy_) { return; }
    Vector3 enemyPos = enemy_->GetCenterPosition();
    float dx = enemyPos.x - targetPos_.x;
    float dz = enemyPos.z - targetPos_.z;
    if (dx * dx + dz * dz <= kAreaRadius_ * kAreaRadius_) {
        enemy_->TakeDamage(static_cast<uint32_t>(kAreaDamage_));
        enemy_->OnRushHit(true);  // 特殊リアクション発動
    }
}

// =============================================================
//  AdvancePhase
// =============================================================
void PlayerUltimate::AdvancePhase(Player* player)
{
    timer_ = 0;
    switch (phase_) {
    case Phase::kRise:    phase_ = Phase::kCharge;  break;
    case Phase::kCharge:  phase_ = Phase::kDive;    break;
    case Phase::kDive:    phase_ = Phase::kImpact;  break;
    case Phase::kImpact:  phase_ = Phase::kRecover; break;
    case Phase::kRecover: phase_ = Phase::kIdle;    break;
    default:              phase_ = Phase::kIdle;    break;
    }
}

// =============================================================
//  ImGui
// =============================================================
void PlayerUltimate::ImGui()
{
#ifdef _DEBUG
    ImGui::Begin("PlayerUltimate");
    const char* names[] = { "Idle","Rise","Charge","Dive","Impact","Recover" };
    ImGui::Text("Phase : %s", names[static_cast<int>(phase_)]);
    ImGui::Text("Timer : %d", timer_);
    ImGui::End();
#endif
}

// =============================================================
//  ApplyVariables
// =============================================================
void PlayerUltimate::ApplyVariables()
{
    kRiseDuration_ = max(variables_->GetIntValue(kGroupName_, "Rise Duration"), 1);
    kChargeDuration_ = max(variables_->GetIntValue(kGroupName_, "Charge Duration"), 1);
    kDiveDuration_ = max(variables_->GetIntValue(kGroupName_, "Dive Duration"), 1);
    kImpactDuration_ = max(variables_->GetIntValue(kGroupName_, "Impact Duration"), 1);
    kRecoverDuration_ = max(variables_->GetIntValue(kGroupName_, "Recover Duration"), 1);
    kRiseHeight_ = variables_->GetFloatValue(kGroupName_, "Rise Height");
    kAreaRadius_ = variables_->GetFloatValue(kGroupName_, "Area Radius");
    kAreaDamage_ = variables_->GetFloatValue(kGroupName_, "Area Damage");
}