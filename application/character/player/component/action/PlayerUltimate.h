#pragma once
#include "GlobalVariables.h"
#include "Vector3.h"
#include <string>
#include <array>

using namespace Engine;
class FollowCamera;

class Player;
class Enemy;

/// <summary>
/// プレイヤー必殺技モーション管理クラス
/// フェーズ: kIdle → kRise → kCharge → kDive → kImpact → kRecover → kIdle
/// </summary>
class PlayerUltimate
{
public:
    enum class Phase {
        kIdle,
        kRise,      // 飛び上がり
        kCharge,    // 空中溜め（右腕を後方へ引く）
        kDive,      // 急降下（右腕を前方へ突き出しながら落下）
        kImpact,    // 着地パンチ＆範囲攻撃
        kRecover,   // 硬直回復（腕を元に戻す）
    };

public:
    PlayerUltimate();

    void Init();
    void Update(Player* player, Enemy* enemy);
    void Start(Player* player, Enemy* enemy);

    bool  IsActive() const { return phase_ != Phase::kIdle; }
    Phase GetPhase() const { return phase_; }

    void ImGui();

private:
    void UpdateRise(Player* player);
    void UpdateCharge(Player* player);
    void UpdateDive(Player* player);      // ★ enemy 引数を削除（enemy_ メンバで保持）
    void UpdateImpact(Player* player);    // ★ enemy 引数を削除
    void UpdateRecover(Player* player);

    void RecalcTarget(Player* player);    // ★ 追加：毎フレーム敵方向・着地点を再計算
    void FaceTarget(Player* player);
    void SaveArmPose(Player* player);
    void RestoreArmPose(Player* player);
    void ApplyAreaDamage(Player* player); // ★ enemy 引数を削除（enemy_ メンバで保持）
    void AdvancePhase(Player* player);
    void ApplyVariables();

private:
    Phase phase_ = Phase::kIdle;
    int   timer_ = 0;
    bool  hasHit_ = false;

    // ★ 追加：敵ポインタを保持（毎フレーム追従用）
    Enemy* enemy_ = nullptr;

    // 座標
    Vector3 startPos_ = {};
    Vector3 peakPos_ = {};
    Vector3 targetPos_ = {};  // Dive の着地目標（毎フレーム更新）

    // 向き制御（毎フレーム再計算）
    float targetRotY_ = 0.0f;

    // 腕の初期姿勢（復帰用）
    std::array<Vector3, 2> savedArmRot_ = {};
    std::array<Vector3, 2> savedArmPos_ = {};

    // GlobalVariables で調整可能なパラメータ
    int   riseDuration_ = 30;
    int   chargeDuration_ = 40;
    int   diveDuration_ = 20;
    int   impactDuration_ = 20;
    int   recoverDuration_ = 30;
    float riseHeight_ = 8.0f;
    float areaRadius_ = 5.0f;
    float areaDamage_ = 300.0f;

    // ★ 腕モーション量（GlobalVariablesで調整可能にしたい場合は Init で登録）
    static constexpr float kChargePullBack = 1.5f;  // Charge で腕を引く距離（ローカル -Z）
    static constexpr float kDivePunchReach = 2.0f;  // Dive で腕を突き出す距離（ローカル +Z）

    GlobalVariables* variables_ = nullptr;
    static const std::string kGroupName_;
};