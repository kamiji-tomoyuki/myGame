#pragma once
#include <array>
#include <memory>
#include "GlobalVariables.h"
#include "PlayerUltGauge.h"
#include "PlayerUltimate.h"

class Player;
class PlayerArm;
class Enemy;

/// <summary>
/// プレイヤーの攻撃ロジックを管理するクラス
/// </summary>
class PlayerAttack
{
public:
    PlayerAttack(Player* player, const std::array<std::unique_ptr<PlayerArm>, 2>& arms);

    void Init();
    void Update();

    bool CanRightPunch() const;
    bool CanLeftPunch()  const;
    bool CanRush()       const;

    bool IsRightPunchActive() const;
    bool IsLeftPunchActive()  const;
    bool IsRushActive()       const;

    // ゲージへのアクセス
    PlayerUltGauge* GetUltGauge() { return &ultGauge_; }
    bool IsUltReady() const { return ultGauge_.IsReady(); }

    // 必殺技アクティブ判定（Player から状態確認に使う）
    bool IsUltimateActive() const { return ultimate_.IsActive(); }
    PlayerUltimate::Phase GetUltimatePhase() const { return ultimate_.GetPhase(); }

    // ImGui（Player::ImGui() から呼ぶ）
    void ImGui();

    // Update を呼ぶ際に Enemy を渡せるようにする
    void UpdateUltimate(Player* player, Enemy* enemy);

    // PlayerArm から命中通知を受け取る
    void OnHit(PlayerUltGauge::HitType type);

private:
    void ApplyVariables();

private:
    Player* player_ = nullptr;
    const std::array<std::unique_ptr<PlayerArm>, 2>* arms_ = nullptr;

    static constexpr int kRArm = 0;
    static constexpr int kLArm = 1;

    bool  comboProtected_ = false;
    int   comboProtectTimer_ = 0;

    // GlobalVariables で調整可能な変数
    int kComboProtectDuration_ = 25;

    GlobalVariables* variables_ = nullptr;
    static const std::string kGroupName_;

    // 必殺技ゲージ
    PlayerUltGauge  ultGauge_;
    // 必殺技モーション
    PlayerUltimate  ultimate_;

    // -------------------------------------------------------
    // ラッシュ交互パンチ制御用定数
    // -------------------------------------------------------
    /// 右腕のラッシュタイマー初期値（位相 0）
    static constexpr uint32_t kRightArmTimerOffset_ = 0;

    /// 左腕オフセットを算出するための除数（2 = 半周期ずれ）
    /// leftOffset = kRushInterval / kAlternateOffsetDivisor_
    static constexpr uint32_t kAlternateOffsetDivisor_ = 2;

    /// PlayerArmRush の interval が取得できなかった場合のフォールバック値
    /// PlayerArmRush::kRushInterval_ のデフォルト値（8）と合わせる
    static constexpr uint32_t kDefaultRushInterval_ = 8;
};