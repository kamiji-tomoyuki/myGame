#pragma once
#include "GlobalVariables.h"
#include <string>

/// <summary>
/// 必殺技ゲージ管理クラス
/// PlayerAttack が保持し、攻撃命中種別ごとに AddGauge() を呼ぶ。
/// </summary>
class PlayerUltGauge
{
public:
    /// <summary>攻撃種別（加算量の選択に使う）</summary>
    enum class HitType {
        kRightPunch,
        kLeftPunch,
        kRush,
        kFinisher,
    };

public:
    PlayerUltGauge();

    /// <summary>初期化 — GlobalVariables グループを登録する</summary>
    void Init();

    /// <summary>毎フレーム呼ぶ（ApplyVariables・クランプ）</summary>
    void Update();

    /// <summary>攻撃命中時にゲージを加算する（種別で加算量を選択）</summary>
    void AddGauge(HitType type);

    /// <summary>デバッグ用：固定値で直接加算（JSON の Gain 値に依存しない）</summary>
    void AddGaugeRaw(float amount);

    /// <summary>必殺技発動時にゲージをリセットする</summary>
    void ConsumeGauge();

    // ----------------------------------------------------------
    // ゲッター
    // ----------------------------------------------------------
    float GetGauge()      const { return gauge_; }
    float GetMaxGauge()   const { return kMaxGauge_; }
    /// <summary>0.0〜1.0 の割合</summary>
    float GetGaugeRatio() const;
    /// <summary>必殺技が発動可能か（kMaxGauge_=0 による誤検知を防ぐ）</summary>
    bool  IsReady()       const;

    // ----------------------------------------------------------
    // ImGui デバッグ表示
    // ----------------------------------------------------------
    void ImGui();

private:
    void ApplyVariables();

private:
    float gauge_ = 0.0f;

    // GlobalVariables で調整可能なパラメータ
    float kMaxGauge_ = 100.0f;
    float kGainRightPunch_ = 10.0f;
    float kGainLeftPunch_ = 10.0f;
    float kGainRush_ = 5.0f;
    float kGainFinisher_ = 20.0f;

    GlobalVariables* variables_ = nullptr;
    static const std::string kGroupName_;
};