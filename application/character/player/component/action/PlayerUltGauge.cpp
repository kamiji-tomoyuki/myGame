#include "PlayerUltGauge.h"
#include <algorithm>
#include <imgui.h>

using namespace Engine;
const std::string PlayerUltGauge::kGroupName_ = "PlayerUltGauge";

// =============================================================
//  コンストラクタ
// =============================================================
PlayerUltGauge::PlayerUltGauge()
{
    variables_ = GlobalVariables::GetInstance();
}

// =============================================================
//  Init
// =============================================================
void PlayerUltGauge::Init()
{
    if (!variables_->GroupExists(kGroupName_)) {
        variables_->CreateGroup(kGroupName_);
    }

    // AddItem は「存在しないキーのみ」デフォルト値で登録する。
    // すでに JSON に値がある場合はそちらが有効になる。
    variables_->AddItem(kGroupName_, "Max Gauge", kMaxGauge_);
    variables_->AddItem(kGroupName_, "Gain RightPunch", kGainRightPunch_);
    variables_->AddItem(kGroupName_, "Gain LeftPunch", kGainLeftPunch_);
    variables_->AddItem(kGroupName_, "Gain Rush", kGainRush_);
    variables_->AddItem(kGroupName_, "Gain Finisher", kGainFinisher_);

    // AddItem 後に必ず読み込んでメンバ変数へ反映する
    ApplyVariables();

    // ゲージは必ずゲーム開始時に 0 から始める
    gauge_ = 0.0f;
}

// =============================================================
//  Update
// =============================================================
void PlayerUltGauge::Update()
{
    // パラメータ（最大値・加算量）だけを毎フレーム更新する。
    // gauge_ の現在値はここでは変更しない。
    // （クランプをここで行うと AddGauge の結果を即座に潰してしまう）
    ApplyVariables();
}

// =============================================================
//  AddGauge — 攻撃命中時 / ImGui ボタンから呼ぶ
// =============================================================
void PlayerUltGauge::AddGauge(HitType type)
{
    float gain = 0.0f;
    switch (type) {
    case HitType::kRightPunch: gain = kGainRightPunch_; break;
    case HitType::kLeftPunch:  gain = kGainLeftPunch_;  break;
    case HitType::kRush:       gain = kGainRush_;       break;
    case HitType::kFinisher:   gain = kGainFinisher_;   break;
    }
    gauge_ = std::clamp(gauge_ + gain, 0.0f, kMaxGauge_);
}

// =============================================================
//  AddGaugeRaw — デバッグ用（固定値で直接加算）
// =============================================================
void PlayerUltGauge::AddGaugeRaw(float amount)
{
    gauge_ = std::clamp(gauge_ + amount, 0.0f, kMaxGauge_);
}

// =============================================================
//  ConsumeGauge
// =============================================================
void PlayerUltGauge::ConsumeGauge()
{
    gauge_ = 0.0f;
}

// =============================================================
//  GetGaugeRatio
// =============================================================
float PlayerUltGauge::GetGaugeRatio() const
{
    if (kMaxGauge_ <= 0.0f) { return 0.0f; }
    return std::clamp(gauge_ / kMaxGauge_, 0.0f, 1.0f);
}

// =============================================================
//  IsReady
// =============================================================
bool PlayerUltGauge::IsReady() const
{
    return (kMaxGauge_ > 0.0f) && (gauge_ >= kMaxGauge_);
}

// =============================================================
//  ImGui
// =============================================================
void PlayerUltGauge::ImGui()
{
#ifdef _DEBUG
    ImGui::Begin("PlayerUltGauge");

    ImGui::Text("Gauge : %.1f / %.1f", gauge_, kMaxGauge_);
    ImGui::ProgressBar(GetGaugeRatio(), ImVec2(-1.0f, 24.0f));

    if (IsReady()) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "*** ULTIMATE READY ***");
    }

    ImGui::Separator();
    ImGui::Text("[Debug]");
    // AddGaugeRaw で固定値を加算（JSON の Gain 値に依存しない）
    if (ImGui::Button("+10")) { AddGaugeRaw(10.0f); }
    ImGui::SameLine();
    if (ImGui::Button("Fill")) { AddGaugeRaw(kMaxGauge_); }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) { ConsumeGauge(); }

    ImGui::End();
#endif
}

// =============================================================
//  ApplyVariables — パラメータのみ更新、gauge_ は触らない
// =============================================================
void PlayerUltGauge::ApplyVariables()
{
    float newMax = variables_->GetFloatValue(kGroupName_, "Max Gauge");

    // kMaxGauge_ が縮小した場合は gauge_ もクランプして整合性を保つ
    if (newMax > 0.0f && newMax != kMaxGauge_) {
        kMaxGauge_ = newMax;
        gauge_ = std::clamp(gauge_, 0.0f, kMaxGauge_);
    }
    else if (newMax > 0.0f) {
        kMaxGauge_ = newMax;
    }
    // newMax <= 0 の場合は前回値を維持（JSON 破損対策）

    kGainRightPunch_ = variables_->GetFloatValue(kGroupName_, "Gain RightPunch");
    kGainLeftPunch_ = variables_->GetFloatValue(kGroupName_, "Gain LeftPunch");
    kGainRush_ = variables_->GetFloatValue(kGroupName_, "Gain Rush");
    kGainFinisher_ = variables_->GetFloatValue(kGroupName_, "Gain Finisher");
}