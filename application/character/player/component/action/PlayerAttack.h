#pragma once
#include <array>
#include <memory>
#include "GlobalVariables.h"
#include "PlayerUltGauge.h"   // ★ 追加

class Player;
class PlayerArm;

/// <summary>
/// プレイヤーの攻撃ロジックを管理するクラス
/// </summary>
class PlayerAttack
{
public:
    PlayerAttack(Player* player, const std::array<std::unique_ptr<PlayerArm>, 2>& arms);

    void Init();   // ★ 追加（ゲージ初期化）
    void Update();

    bool CanRightPunch() const;
    bool CanLeftPunch()  const;
    bool CanRush()       const;

    bool IsRightPunchActive() const;
    bool IsLeftPunchActive()  const;
    bool IsRushActive()       const;

    // ★ 追加 — ゲージへのアクセス
    PlayerUltGauge* GetUltGauge() { return &ultGauge_; }
    bool IsUltReady() const { return ultGauge_.IsReady(); }

    // ★ 追加 — ImGui（Player::ImGui() から呼ぶ）
    void ImGui();

    // ★ 追加 — PlayerArm から命中通知を受け取る
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

    // ★ 追加 — 必殺技ゲージ
    PlayerUltGauge ultGauge_;
};