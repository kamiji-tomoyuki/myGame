#include "DebugScene.h"
#include "SceneManager.h"
#include "ImGuiManager.h"
#include <imgui.h>
#include "GlobalVariables.h"
#include "line/DrawLine3D.h"
#include "Frame.h"
#include "TextTextureBaker.h"
#include "TextureManager.h"
#include "SrvManager.h"
#ifdef _DEBUG
#include "EditorUI.h"
#endif // _DEBUG

#include "myMath.h"
#include "Quaternion.h"
#include "Matrix4x4.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <json.hpp>

using namespace Engine;

void DebugScene::Initialize() {
    audio_ = Audio::GetInstance();
    objCommon_ = Object3dCommon::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();

    vp_.Initialize();
    vp_.translation_ = { 0.0f, 2.0f, -12.0f };
    vp_.rotation_ = { 0.12f, 0.0f, 0.0f };

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);
    debugCamera_->SetActive(false);

    skybox_ = std::make_unique<Skybox>();
    skybox_->Initialize("skybox.dds");

    // パーティクル
    emitter_ = std::make_unique<ParticleEmitter>();
    emitter_->Initialize(particleName_, modelPath_);

    // モーション プレビューリグ
    InitRig();
    InitRushPreview();

    // テキストエディタ
    ScanFonts();
    InitTextEditor();

    // 既定姿勢（実プレイヤーの初期腕位置に合わせる）
    pose_[0] = PartPose{};                 // 体
    pose_[1].translate = rArmBase_;        // 右腕
    pose_[2].translate = lArmBase_;        // 左腕

    clip_.SetName(clipName_);
    ScanClips();

    // combo_list.json に保存済みの補間時間があればスライダの初期値に反映
    try {
        std::ifstream ifs("resources/jsons/motion/combo_list.json");
        if (ifs) {
            nlohmann::json r; ifs >> r;
            if (r.is_object()) { comboBlendTime_ = r.value("blendTime", comboBlendTime_); }
        }
    } catch (...) {}

    UpdateRig();
    SetCameraPreset(0); // 既定は正面カメラ
}

void DebugScene::Finalize() {}

void DebugScene::InitRig() {
    bodyObj_ = std::make_unique<Object3d>();
    bodyObj_->Initialize("Player/playerBody.obj");

    // 左右の腕はそれぞれ独立生成（Object3dの変換用CBが1つのため使い回すと重なる）
    rArmObj_ = std::make_unique<Object3d>();
    rArmObj_->Initialize("player/Arm/playerArm.gltf");

    lArmObj_ = std::make_unique<Object3d>();
    lArmObj_->Initialize("player/Arm/playerArm.gltf");

    bodyTf_.Initialize();
    rArmTf_.Initialize();
    lArmTf_.Initialize();
    rArmTf_.parent_ = &bodyTf_;
    lArmTf_.parent_ = &bodyTf_;
}

// =============================================================
//  更新
// =============================================================
void DebugScene::Update() {
#ifdef _DEBUG
    Debug();
#endif
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }

    skybox_->Update(vp_);

    if (editorMode_ == 0) {
        // パーティクル
        if (emitter_) { emitter_->Update(vp_); }
    } else if (editorMode_ == 2) {
        // ラッシュ
        UpdateRushPreview();
    } else if (editorMode_ == 3) {
        // テキスト
        UpdateTextEditor();
    } else {
        // モーション
        UpdateMotionPlayback();
        UpdateRig();
    }

    if (input_->TriggerKey(DIK_ESCAPE)) {
        sceneManager_->NextSceneReservation("TITLE");
    }
}

void DebugScene::UpdateMotionPlayback() {
    // コンボ再生プレビュー（コンボの連続再生）が最優先
    if (comboPlaying_ && comboPreview_) {
        comboPreview_->Update(Frame::DeltaTime());
        if (comboPreview_->IsActive()) {
            // 体・腕すべてクリップ通りに駆動（体のひねりもパンチ演出として表示）
            pose_[0] = comboPreview_->GetBodyPose();
            pose_[1] = comboPreview_->GetRArmPose();
            pose_[2] = comboPreview_->GetLArmPose();
        } else {
            comboPlaying_ = false;
        }
        return;
    }

    // 単体クリップ再生
    if (!playing_) { return; }

    const float duration = clip_.GetDuration();
    editTime_ += Frame::DeltaTime();
    if (editTime_ >= duration) {
        if (loop_) {
            editTime_ = std::fmod(editTime_, duration);
        } else {
            editTime_ = duration;
            playing_ = false;
        }
    }
    ApplyPoseFromClip(editTime_);
}

void DebugScene::ApplyPoseFromClip(float t) {
    pose_[0] = clip_.SampleBody(t);
    pose_[1] = clip_.SampleRArm(t);
    pose_[2] = clip_.SampleLArm(t);
}

void DebugScene::UpdateRig() {
    // 体（親）→ 腕（子）の順で行列更新
    bodyTf_.translation_ = pose_[0].translate;
    bodyTf_.rotation_ = pose_[0].rotate;
    bodyTf_.scale_ = { 1.0f, 1.0f, 1.0f };
    bodyTf_.UpdateMatrix();

    rArmTf_.translation_ = pose_[1].translate;
    rArmTf_.rotation_ = pose_[1].rotate;
    rArmTf_.scale_ = armScale_;
    rArmTf_.UpdateMatrix();

    lArmTf_.translation_ = pose_[2].translate;
    lArmTf_.rotation_ = pose_[2].rotate;
    lArmTf_.scale_ = armScale_;
    lArmTf_.UpdateMatrix();

    if (rArmObj_) { rArmObj_->UpdateAnimation(true); }
    if (lArmObj_) { lArmObj_->UpdateAnimation(true); }
}

void DebugScene::CaptureKeyframe() {
    // 現在の姿勢を editTime_ のキーとして登録（同時刻付近は上書き）。
    // 追加/上書き/ソート/duration整合は PlayerMotionClip 内部で処理する。
    clip_.UpsertKeyframe(editTime_, pose_[0], pose_[1], pose_[2]);
}

void DebugScene::ScanClips() {
    clipFiles_.clear();
    try {
        std::filesystem::path root = "resources/jsons/motion";
        if (std::filesystem::exists(root)) {
            for (const auto& e : std::filesystem::directory_iterator(root)) {
                if (e.is_regular_file() && e.path().extension() == ".json") {
                    const std::string stem = e.path().stem().string();
                    // コンボ定義ファイルはクリップではないので一覧から除外
                    if (stem == "combo_list") { continue; }
                    clipFiles_.push_back(stem);
                }
            }
        }
    } catch (...) {}
    std::sort(clipFiles_.begin(), clipFiles_.end());
}

void DebugScene::SetCameraPreset(int preset) {
    // プリセットは vp_ を直接設定するため、デバッグカメラを無効化して固定する。
    debugCamera_->SetActive(false);

    // キャラの正面は +Z 側（腕が +Z へ伸びる）。カメラは local +Z を向く。
    constexpr float kPi = 3.14159265358979323846f;
    switch (preset) {
    case 0: // 正面（+Z側から -Z を見る）
        vp_.translation_ = { 0.0f, 2.0f, 12.0f };
        vp_.rotation_ = { 0.1f, kPi, 0.0f };
        break;
    case 1: // 背面（-Z側から +Z を見る）
        vp_.translation_ = { 0.0f, 2.0f, -12.0f };
        vp_.rotation_ = { 0.1f, 0.0f, 0.0f };
        break;
    case 2: // 横（+X側から -X を見る）
        vp_.translation_ = { 12.0f, 2.0f, 0.0f };
        vp_.rotation_ = { 0.1f, -kPi * 0.5f, 0.0f };
        break;
    case 3: { // 俯瞰（ゲーム中の追従カメラと同じ見下ろし角を再現）
        // FollowCamera::Update と同じ計算（正しいクォータニオン経路）。
        //   回転 = MakeRotateAxisAngle(kAxisZ,-2.9) * MakeRotateAxisAngle(kAxisY, angleY)
        //   位置 = ターゲット(原点) + オフセット{0,2,-10} を回転で変換
        const Vector3 kAxisZ = { 0.0f, 0.0f, -1.0f };
        const Vector3 kAxisY = { 0.0f, -1.0f, 0.0f };
        const float   kPitch = -2.9f;   // FollowCamera::kDefaultPitchAngle
        const float   angleY = 0.0f;    // キャラ正面(+Z, rotation.y=0)に対応
        Quaternion q =
            Quaternion::MakeRotateAxisAngleQuaternion(kAxisZ, kPitch) *
            Quaternion::MakeRotateAxisAngleQuaternion(kAxisY, angleY);
        vp_.rotation_ = q.ToEulerAngles();
        Matrix4x4 rotM = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, vp_.rotation_, {});
        vp_.translation_ = TransformNormal({ 0.0f, 2.0f, -10.0f }, rotM); // FollowCamera::kDefaultOffset
        break;
    }
    default:
        break;
    }
    vp_.UpdateMatrix();
}

// コンボ定義（combo_list.json）のクリップ順を取得（配列形式・オブジェクト形式の両対応）
std::vector<std::string> DebugScene::ReadComboClips() const {
    using json = nlohmann::json;
    std::vector<std::string> result;
    try {
        std::ifstream ifs("resources/jsons/motion/combo_list.json");
        if (!ifs) { return result; }
        json root; ifs >> root;
        const json* names = nullptr;
        if (root.is_array()) { names = &root; }
        else if (root.is_object() && root.contains("clips") && root["clips"].is_array()) { names = &root["clips"]; }
        if (names) {
            for (const auto& n : *names) { if (n.is_string()) { result.push_back(n.get<std::string>()); } }
        }
    } catch (...) {}
    return result;
}

// コンボ定義を combo_list.json（クリップ順＋補間時間）で書き出す。
// clip 保存ファイル(<name>.json)と衝突しない専用ファイル名を使う。
void DebugScene::WriteComboJson(const std::vector<std::string>& clips) {
    using json = nlohmann::json;
    try {
        std::filesystem::create_directories("resources/jsons/motion");
        json root;
        root["blendTime"] = comboBlendTime_;
        root["clips"] = json::array();
        for (const auto& c : clips) { root["clips"].push_back(c); }
        std::ofstream ofs("resources/jsons/motion/combo_list.json");
        if (ofs) { ofs << root.dump(2); }
    } catch (...) {}
}

void DebugScene::AppendClipToCombo() {
    std::vector<std::string> clips = ReadComboClips();
    const std::string name = clipName_;
    if (std::find(clips.begin(), clips.end(), name) == clips.end()) {
        clips.push_back(name);
    }
    WriteComboJson(clips);
}

// combo.json のコンボを連続再生（クリップ間はブレンド補間、任意でループ）
void DebugScene::StartComboPreview() {
    comboPreview_ = std::make_unique<PlayerComboMotion>();
    comboPreview_->SetAutoAdvance(comboAutoAdvance_); // オフなら手動入力（受付時間テスト）
    comboPreview_->SetLoop(comboLoop_);
    comboPreview_->Init();                     // combo.json ＋ クリップを読み込み
    comboPreview_->SetBlendTime(comboBlendTime_); // UI の補間時間を優先反映
    comboPreview_->StartFromBeginning();
    comboPlaying_ = true;
    playing_ = false; // 単体再生は停止
}

// =============================================================
//  描画
// =============================================================
void DebugScene::Draw() {
    skybox_->Draw();

    if (editorMode_ == 0) {
        // パーティクル
        ptCommon_->DrawCommonSetting();
        if (emitter_) {
            emitter_->Draw(static_cast<PrimitiveType>(currentPrimitive_));
            emitter_->DrawEmitter();
        }
    } else if (editorMode_ == 2) {
        // ラッシュプレビュー
        DrawRushPreview();
    } else if (editorMode_ == 3) {
        // テキスト仮描画
        DrawTextPreview();
    } else {
        // モーションリグ
        DrawRig();
    }

    DrawLine3D::GetInstance()->Draw(vp_);
}

void DebugScene::DrawRig() {
    // 腕（gltf / スキニング）— 左右それぞれ独立したObject3dで描画
    objCommon_->skinningDrawCommonSetting();
    if (rArmObj_) { rArmObj_->Draw(rArmTf_, vp_); }
    if (lArmObj_) { lArmObj_->Draw(lArmTf_, vp_); }

    // 体（obj / 通常）
    objCommon_->DrawCommonSetting();
    if (bodyObj_) { bodyObj_->Draw(bodyTf_, vp_); }
}

// =============================================================
//  ラッシュプレビュー
// =============================================================
void DebugScene::InitRushPreview() {
    rushR_ = std::make_unique<PlayerArmRush>();
    rushL_ = std::make_unique<PlayerArmRush>();
    for (int i = 0; i < kRushTrail_; ++i) {
        trailRushR_[i] = std::make_unique<PlayerArmRush>();
        trailRushL_[i] = std::make_unique<PlayerArmRush>();
        trailArmR_[i] = std::make_unique<Object3d>();
        trailArmR_[i]->Initialize("player/Arm/playerArm.gltf");
        trailArmL_[i] = std::make_unique<Object3d>();
        trailArmL_[i]->Initialize("player/Arm/playerArm.gltf");
        trailTfR_[i].Initialize(); trailTfR_[i].parent_ = &bodyTf_;
        trailTfL_[i].Initialize(); trailTfL_[i].parent_ = &bodyTf_;
    }
    finisherPreview_ = std::make_unique<PlayerComboMotion>();
    finisherPreview_->InitSingle("finisher");
}

void DebugScene::StartRushPreview() {
    const uint32_t interval = rushR_->GetRushInterval();
    const uint32_t leftOffset = interval / 2;
    rushR_->StartRush(true, rArmBase_, 0);
    rushL_->StartRush(false, lArmBase_, leftOffset);
    for (int i = 0; i < kRushTrail_; ++i) {
        const uint32_t off = static_cast<uint32_t>((i + 1) * (rushTrailStep_ > 0 ? rushTrailStep_ : 1));
        trailRushR_[i]->StartRush(true, rArmBase_, off);
        trailRushL_[i]->StartRush(false, lArmBase_, leftOffset + off);
    }
    if (finisherPreview_) { finisherPreview_->InitSingle("finisher"); finisherPreview_->Stop(); }
    rushFinStarted_ = false;
    rushPlaying_ = true;
    // 体は連打中は中立
    pose_[0] = PartPose{};
}

void DebugScene::UpdateRushPreview() {
    // 体は連打中は中立、フィニッシャー中はクリップの体回転
    bodyTf_.translation_ = pose_[0].translate;
    bodyTf_.rotation_ = pose_[0].rotate;
    bodyTf_.scale_ = { 1.0f, 1.0f, 1.0f };
    bodyTf_.UpdateMatrix();

    if (rushPlaying_ && rushR_) {
        if (rushR_->GetIsRush()) {
            // --- 連打フェーズ：PlayerArmRush で主腕＋残像を駆動 ---
            rushR_->Update(0.0f);
            rushL_->Update(0.0f);
            rArmTf_.translation_ = rushR_->GetCurrentTranslation();
            rArmTf_.rotation_ = rushR_->GetCurrentRotation();
            lArmTf_.translation_ = rushL_->GetCurrentTranslation();
            lArmTf_.rotation_ = rushL_->GetCurrentRotation();
            for (int i = 0; i < kRushTrail_; ++i) {
                trailRushR_[i]->Update(0.0f);
                trailRushL_[i]->Update(0.0f);
                trailTfR_[i].translation_ = trailRushR_[i]->GetCurrentTranslation();
                trailTfR_[i].rotation_ = trailRushR_[i]->GetCurrentRotation();
                trailTfL_[i].translation_ = trailRushL_[i]->GetCurrentTranslation();
                trailTfL_[i].rotation_ = trailRushL_[i]->GetCurrentRotation();
            }
            pose_[0] = PartPose{};
        }
        else if (rushR_->IsRapidPunchDone()) {
            // --- フィニッシャークリップ ---
            if (!rushFinStarted_) { finisherPreview_->StartFromBeginning(); rushFinStarted_ = true; }
            finisherPreview_->Update(Frame::DeltaTime());
            if (finisherPreview_->IsActive()) {
                pose_[0] = finisherPreview_->GetBodyPose();
                rArmTf_.translation_ = finisherPreview_->GetRArmPose().translate;
                rArmTf_.rotation_ = finisherPreview_->GetRArmPose().rotate;
                lArmTf_.translation_ = finisherPreview_->GetLArmPose().translate;
                lArmTf_.rotation_ = finisherPreview_->GetLArmPose().rotate;
            } else {
                if (rushLoop_) { StartRushPreview(); }
                else { rushPlaying_ = false; }
            }
        }
    }

    // 行列更新
    rArmTf_.scale_ = armScale_;
    lArmTf_.scale_ = armScale_;
    rArmTf_.UpdateMatrix();
    lArmTf_.UpdateMatrix();
    if (rArmObj_) { rArmObj_->UpdateAnimation(true); }
    if (lArmObj_) { lArmObj_->UpdateAnimation(true); }
    for (int i = 0; i < kRushTrail_; ++i) {
        trailTfR_[i].scale_ = armScale_;
        trailTfL_[i].scale_ = armScale_;
        trailTfR_[i].UpdateMatrix();
        trailTfL_[i].UpdateMatrix();
        if (trailArmR_[i]) { trailArmR_[i]->UpdateAnimation(true); }
        if (trailArmL_[i]) { trailArmL_[i]->UpdateAnimation(true); }
    }
}

void DebugScene::DrawRushPreview() {
    objCommon_->skinningDrawCommonSetting();

    // 残像（連打フェーズのみ、後ろほど薄く）
    const bool rapid = rushPlaying_ && rushR_ && rushR_->GetIsRush();
    if (rapid) {
        float alpha = rushTrailAlpha_;
        for (int i = 0; i < kRushTrail_; ++i) {
            ObjColor col; col.SetColor({ 1.0f, 1.0f, 1.0f, alpha });
            if (trailArmR_[i]) { trailArmR_[i]->Draw(trailTfR_[i], vp_, &col); }
            if (trailArmL_[i]) { trailArmL_[i]->Draw(trailTfL_[i], vp_, &col); }
            alpha *= rushTrailFalloff_;
        }
    }

    // 主腕
    if (rArmObj_) { rArmObj_->Draw(rArmTf_, vp_); }
    if (lArmObj_) { lArmObj_->Draw(lArmTf_, vp_); }

    // 体
    objCommon_->DrawCommonSetting();
    if (bodyObj_) { bodyObj_->Draw(bodyTf_, vp_); }
}

void DebugScene::DrawRushUI() {
#ifdef _DEBUG
    EditorUI* editor = EditorUI::GetInstance();
    if (!editor->PanelVisible("ラッシュプレビュー", "ラッシュ")) { return; }

    ImGui::Begin("Rush Preview");

    ImGui::TextUnformatted("再生");
    if (ImGui::Button("▶ ラッシュ再生")) { StartRushPreview(); }
    ImGui::SameLine();
    if (ImGui::Button("停止")) { rushPlaying_ = false; }
    ImGui::Checkbox("ループ", &rushLoop_);
    {
        const bool rapid = rushPlaying_ && rushR_ && rushR_->GetIsRush();
        const bool fin = rushPlaying_ && finisherPreview_ && finisherPreview_->IsActive();
        ImGui::TextDisabled("状態: %s", !rushPlaying_ ? "停止" : (rapid ? "連打" : (fin ? "フィニッシャー" : "-")));
    }

    ImGui::Separator();
    ImGui::TextUnformatted("残像（プレビュー表示）");
    ImGui::SliderFloat("先頭アルファ", &rushTrailAlpha_, 0.0f, 1.0f);
    ImGui::SliderFloat("減衰率", &rushTrailFalloff_, 0.1f, 1.0f);
    ImGui::SliderInt("オフセット間隔", &rushTrailStep_, 1, 12);
    ImGui::TextDisabled("本数=%d（片側）。ゲーム本番の本数/透明度はGlobalVariablesのPlayerRushTrail", kRushTrail_);

    ImGui::Separator();
    ImGui::TextUnformatted("調整値の保存");
    ImGui::TextDisabled("値の変更は Global Variables ウィンドウで行う");
    if (ImGui::Button("ラッシュ設定を保存 (PlayerArmRush)")) {
        GlobalVariables::GetInstance()->SaveFile("PlayerArmRush");
    }
    if (ImGui::Button("残像設定を保存 (PlayerRushTrail)")) {
        GlobalVariables::GetInstance()->SaveFile("PlayerRushTrail");
    }
    ImGui::TextDisabled("フィニッシャーは「モーション」モードで clip名 finisher を編集・保存");

    ImGui::End();
#endif // _DEBUG
}

void DebugScene::DrawForOffScreen() {}

// =============================================================
//  ImGui
// =============================================================
void DebugScene::Debug() {
#ifdef _DEBUG
    EditorUI* editor = EditorUI::GetInstance();

    if (editor->PanelVisible("デバッグシーン", "デバッグ")) {
        ImGui::Begin("Debug Scene");

        if (ImGui::Button("Back to Title")) {
            sceneManager_->NextSceneReservation("TITLE");
        }
        ImGui::SameLine();
        bool dbgCam = debugCamera_->GetActive();
        if (ImGui::Checkbox("Debug Camera", &dbgCam)) {
            debugCamera_->SetActive(dbgCam);
        }

        ImGui::Separator();
        ImGui::TextUnformatted("カメラ");
        if (ImGui::Button("正面")) { SetCameraPreset(0); }
        ImGui::SameLine();
        if (ImGui::Button("背面")) { SetCameraPreset(1); }
        ImGui::SameLine();
        if (ImGui::Button("横")) { SetCameraPreset(2); }
        ImGui::SameLine();
        if (ImGui::Button("俯瞰")) { SetCameraPreset(3); }

        ImGui::Separator();
        ImGui::TextUnformatted("モード");
        ImGui::RadioButton("パーティクル", &editorMode_, 0);
        ImGui::SameLine();
        ImGui::RadioButton("モーション", &editorMode_, 1);
        ImGui::SameLine();
        ImGui::RadioButton("ラッシュ", &editorMode_, 2);
        ImGui::SameLine();
        ImGui::RadioButton("テキスト", &editorMode_, 3);

        ImGui::End();
    }

    if (editorMode_ == 0) {
        DrawParticleUI();
    } else if (editorMode_ == 2) {
        DrawRushUI();
    } else if (editorMode_ == 3) {
        DrawTextUI();
    } else {
        DrawMotionUI();
    }
#endif // _DEBUG
}

void DebugScene::DrawParticleUI() {
#ifdef _DEBUG
    EditorUI* editor = EditorUI::GetInstance();

    if (editor->PanelVisible("パーティクル設定", "パーティクル")) {
        ImGui::Begin("Particle Editor Scene");

        ImGui::InputText("Particle Name", particleName_, sizeof(particleName_));
        ImGui::InputText("Model Path", modelPath_, sizeof(modelPath_));

        const char* primitiveNames[] = { "Normal", "Ring", "Cylinder" };
        ImGui::Combo("Primitive Type", &currentPrimitive_, primitiveNames, IM_ARRAYSIZE(primitiveNames));

        if (ImGui::Button("Reset Emitter")) {
            emitter_ = std::make_unique<ParticleEmitter>();
            emitter_->Initialize(particleName_, modelPath_);
        }

        if (emitter_) {
            if (ImGui::Button("Save to JSON")) {
                GlobalVariables::GetInstance()->SaveFile(particleName_);
                std::string message = std::format("{}.json saved.", particleName_);
                MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
            }
        }

        ImGui::End();
    }

    // パーティクル(emitter_)は集約「パーティクル」窓で編集する
#endif // _DEBUG
}

void DebugScene::DrawMotionUI() {
#ifdef _DEBUG
    EditorUI* editor = EditorUI::GetInstance();
    if (!editor->PanelVisible("モーションエディタ", "モーション")) { return; }

    ImGui::Begin("Motion Editor");

    // ---- クリップ管理 ----
    if (ImGui::CollapsingHeader("クリップ", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputText("名前", clipName_, sizeof(clipName_));
        if (ImGui::Button("新規")) {
            clip_ = PlayerMotionClip{};
            clip_.SetName(clipName_);
            editTime_ = 0.0f;
            playing_ = false;
            comboPlaying_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("保存")) {
            // コンボ定義ファイルと衝突する予約名は弾く
            std::string nm = clipName_;
            if (nm == "combo" || nm == "combo_list") {
                MessageBoxA(nullptr, "その名前は予約済みです。別のクリップ名を使ってください。", "Motion Editor", 0);
            } else {
                clip_.SetName(clipName_);
                clip_.Save();
                ScanClips();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("読込")) {
            comboPlaying_ = false;
            if (clip_.Load(clipName_)) {
                editTime_ = 0.0f;
                ApplyPoseFromClip(0.0f);
            }
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("再スキャン")) { ScanClips(); }

        if (ImGui::Button("コンボに登録 (combo_list.json)")) {
            clip_.SetName(clipName_);
            clip_.Save();          // 未保存でも確実にファイル化
            AppendClipToCombo();   // combo.json 末尾へ追加（プレイヤーが読み込む）
            ScanClips();
        }

        // 左右対称保存：現在のクリップを鏡像化して別名で保存（右パンチ→左パンチ等）
        ImGui::Separator();
        ImGui::InputText("反転名", mirrorName_, sizeof(mirrorName_));
        ImGui::SameLine();
        if (ImGui::Button("左右反転して保存")) {
            PlayerMotionClip mirrored = clip_.MakeMirrored(mirrorName_);
            mirrored.Save();
            ScanClips();
        }

        // ラッシュの最後の一撃として保存（finisher.json）。ゲーム/ラッシュプレビューが読む。
        if (ImGui::Button("フィニッシャーとして保存 (finisher.json)")) {
            clip_.SetName("finisher");
            clip_.Save();
            strncpy_s(clipName_, "finisher", _TRUNCATE);
            ScanClips();
        }

        ImGui::BeginChild("ClipList", ImVec2(0.0f, 90.0f), true);
        for (const auto& name : clipFiles_) {
            bool selected = (name == clipName_);
            if (ImGui::Selectable(name.c_str(), selected)) {
                comboPlaying_ = false;
                strncpy_s(clipName_, name.c_str(), _TRUNCATE);
                if (clip_.Load(name)) { editTime_ = 0.0f; ApplyPoseFromClip(0.0f); }
            }
        }
        ImGui::EndChild();
    }

    // ---- コンボ再生（combo.json を連続再生、クリップ間はブレンド補間） ----
    if (ImGui::CollapsingHeader("コンボ再生", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderFloat("補間時間(秒)", &comboBlendTime_, 0.0f, 1.0f)) {
            if (comboPreview_) { comboPreview_->SetBlendTime(comboBlendTime_); }
        }
        if (ImGui::Checkbox("ループ##combo", &comboLoop_)) {
            if (comboPreview_) { comboPreview_->SetLoop(comboLoop_); }
        }
        // オートで連鎖 / 手動入力（受付時間の効果を確認するには手動にする）
        if (ImGui::Checkbox("オートで連鎖", &comboAutoAdvance_)) {
            if (comboPreview_) { comboPreview_->SetAutoAdvance(comboAutoAdvance_); }
        }
        ImGui::SameLine(); ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("オフ: クリップ再生中に『コンボ入力』を押した時、\n各クリップの『コンボ受付開始(秒)』以降でのみ次段へ連鎖します。\n受付時間より前に押しても繋がりません＝受付時間の効果を確認できます。");
        }
        if (ImGui::Button("コンボを再生")) { StartComboPreview(); }
        ImGui::SameLine();
        if (ImGui::Button("停止")) { comboPlaying_ = false; }
        // 手動コンボ入力（ゲームの攻撃ボタン相当）。受付窓内なら次段へ、最終段なら"ラッシュ"扱い。
        if (ImGui::Button("コンボ入力(次段へ)")) {
            if (comboPreview_ && comboPlaying_) { comboPreview_->TryAdvance(); }
        }
        if (ImGui::Button("補間時間を保存 (combo_list.json)")) { WriteComboJson(ReadComboClips()); }
        if (comboPlaying_ && comboPreview_) {
            ImGui::TextDisabled("コンボ再生中… 段=%d/%d",
                comboPreview_->GetCurrentIndex() + 1, comboPreview_->GetClipCount());
        }
    }

    // ---- パート姿勢編集 ----
    if (ImGui::CollapsingHeader("パーツ編集", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* parts[] = { "体", "右腕", "左腕" };
        ImGui::Combo("パーツ", &selectedPart_, parts, IM_ARRAYSIZE(parts));
        // 姿勢を手編集したらコンボ再生は止める（プレビューに上書きされないように）
        if (ImGui::DragFloat3("座標", &pose_[selectedPart_].translate.x, 0.02f)) { comboPlaying_ = false; }
        if (ImGui::DragFloat3("回転", &pose_[selectedPart_].rotate.x, 0.01f)) { comboPlaying_ = false; }
        if (ImGui::Button("この姿勢をリセット")) {
            Vector3 base = (selectedPart_ == 1) ? rArmBase_ : (selectedPart_ == 2) ? lArmBase_ : Vector3{ 0,0,0 };
            pose_[selectedPart_].translate = base;
            pose_[selectedPart_].rotate = { 0.0f, 0.0f, 0.0f };
        }
    }

    // ---- タイムライン ----
    if (ImGui::CollapsingHeader("タイムライン", ImGuiTreeNodeFlags_DefaultOpen)) {
        float duration = clip_.GetDuration();
        if (ImGui::DragFloat("長さ(秒)", &duration, 0.02f, 0.05f, 20.0f)) {
            clip_.SetDuration(duration);
        }
        // 時刻スライダは「時刻カーソルの移動のみ」。編集中の姿勢は上書きしない
        // （以前はここで補間サンプリングして pose_ を上書きし、編集が消えていた）。
        if (ImGui::SliderFloat("時刻", &editTime_, 0.0f, clip_.GetDuration())) {
            playing_ = false;
            comboPlaying_ = false;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("この時刻を表示")) {
            // 明示的に押したときだけ補間結果を pose_ に反映する
            playing_ = false;
            comboPlaying_ = false;
            ApplyPoseFromClip(editTime_);
        }
        ImGui::TextDisabled("スライダは時刻移動のみ。姿勢は保持されます");

        if (ImGui::Checkbox("再生", &playing_)) { if (playing_) { comboPlaying_ = false; } }
        ImGui::SameLine();
        ImGui::Checkbox("ループ", &loop_);

        if (ImGui::Button("現在の姿勢をキー追加/更新")) {
            comboPlaying_ = false;
            CaptureKeyframe();
        }

        ImGui::TextDisabled("キーフレーム (%d)", static_cast<int>(clip_.Keys().size()));
        ImGui::BeginChild("KeyList", ImVec2(0.0f, 110.0f), true);
        int removeIdx = -1;
        const auto& keys = clip_.Keys(); // 表示は読み取り専用アクセス
        for (int i = 0; i < static_cast<int>(keys.size()); ++i) {
            ImGui::PushID(i);
            char label[64];
            snprintf(label, sizeof(label), "%d: t=%.2f", i, keys[i].time);
            if (ImGui::Selectable(label, false, ImGuiSelectableFlags_AllowItemOverlap)) {
                editTime_ = keys[i].time;
                playing_ = false;
                comboPlaying_ = false;
                ApplyPoseFromClip(editTime_);
            }
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 40.0f);
            if (ImGui::SmallButton("削除")) { removeIdx = i; }
            ImGui::PopID();
        }
        if (removeIdx >= 0) {
            clip_.RemoveKeyframe(static_cast<size_t>(removeIdx)); // 削除もクラスに委譲
        }
        ImGui::EndChild();
    }

    // ---- 戦闘メタ ----
    // ヒット窓・コンボ受付開始は内部では正規化[0,1]だが、UIでは「秒」で編集する。
    // スライダ最大値＝そのモーションの長さ(duration)なので、実時間で直感的に指定できる。
    if (ImGui::CollapsingHeader("戦闘設定")) {
        const float dur = clip_.GetDuration();
        auto toSec   = [&](float n) { return n * dur; };
        auto toNorm  = [&](float s) { return dur > 0.0f ? (s / dur) : 0.0f; };

        float hitStartSec = toSec(clip_.GetHitStart());
        float hitEndSec   = toSec(clip_.GetHitEnd());
        if (ImGui::SliderFloat("ヒット開始(秒)", &hitStartSec, 0.0f, dur, "%.3f")) { clip_.SetHitStart(toNorm(hitStartSec)); }
        if (ImGui::SliderFloat("ヒット終了(秒)", &hitEndSec, 0.0f, dur, "%.3f")) { clip_.SetHitEnd(toNorm(hitEndSec)); }

        int hitArm = static_cast<int>(clip_.GetHitArm());
        const char* arms[] = { "右腕", "左腕", "両方" };
        if (ImGui::Combo("ヒット腕", &hitArm, arms, IM_ARRAYSIZE(arms))) {
            clip_.SetHitArm(static_cast<HitArm>(hitArm));
        }
        int damage = static_cast<int>(clip_.GetDamage());
        if (ImGui::InputInt("ダメージ", &damage)) {
            clip_.SetDamage(static_cast<uint32_t>(damage < 0 ? 0 : damage));
        }
        float cwSec = toSec(clip_.GetComboWindowStart());
        if (ImGui::SliderFloat("コンボ受付開始(秒)", &cwSec, 0.0f, dur, "%.3f")) { clip_.SetComboWindowStart(toNorm(cwSec)); }
    }

    ImGui::End();
#endif // _DEBUG
}

// =============================================================
//  テキストエディタ
// =============================================================
void DebugScene::ScanFonts() {
    fontFiles_.clear();
    try {
        std::filesystem::path root = "resources/fonts";
        if (std::filesystem::exists(root)) {
            for (const auto& e : std::filesystem::directory_iterator(root)) {
                if (!e.is_regular_file()) { continue; }
                std::string ext = e.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (ext == ".ttf" || ext == ".otf") {
                    fontFiles_.push_back(e.path().generic_string()); // "resources/fonts/xxx.ttf"
                }
            }
        }
    } catch (...) {}
    std::sort(fontFiles_.begin(), fontFiles_.end());

    // 日本語対応（PixelMplus）を既定に選んでおく
    for (int i = 0; i < static_cast<int>(fontFiles_.size()); ++i) {
        if (fontFiles_[i].find("Mplus") != std::string::npos || fontFiles_[i].find("mplus") != std::string::npos) {
            textFontIndex_ = i;
            break;
        }
    }
    if (textFontIndex_ >= static_cast<int>(fontFiles_.size())) { textFontIndex_ = 0; }
}

std::string DebugScene::CurrentFontPath() const {
    if (fontFiles_.empty()) { return "resources/fonts/PixelMplus12-Regular.ttf"; }
    int idx = textFontIndex_;
    if (idx < 0 || idx >= static_cast<int>(fontFiles_.size())) { idx = 0; }
    return fontFiles_[idx];
}

std::string DebugScene::FallbackFontPath() const {
    if (!textAutoFallback_) { return ""; }
    const std::string current = CurrentFontPath();
    // 日本語グリフを持つフォント（PixelMplus）を補完用に探す
    for (const auto& f : fontFiles_) {
        if ((f.find("Mplus") != std::string::npos || f.find("mplus") != std::string::npos) && f != current) {
            return f;
        }
    }
    return "";
}

TextRenderParams DebugScene::BuildTextParams() const {
    TextRenderParams p;
    p.text = textBuf_;
    p.fontPath = CurrentFontPath();
    p.fallbackFontPath = FallbackFontPath();
    p.pixelSize = textSize_;
    p.color = { textColor_[0], textColor_[1], textColor_[2], textColor_[3] };
    p.outlineEnabled = textOutline_;
    p.outlineColor = { textOutlineColor_[0], textOutlineColor_[1], textOutlineColor_[2], textOutlineColor_[3] };
    p.outlineWidth = textOutlineWidth_;
    p.lineSpacing = textLineSpacing_;
    p.alignment = textAlign_;
    p.padding = 6;
    return p;
}

void DebugScene::InitTextEditor() {
    textPreview_ = std::make_unique<TextSprite>();
    textPreview_->Initialize(BuildTextParams(), { textPos_[0], textPos_[1] }, { 0.5f, 0.5f });
    textPreview_->SetScale(textScale_);
    textPreview_->SetColorMultiply({ textMul_[0], textMul_[1], textMul_[2], textMul_[3] });
    textDirty_ = false;
}

void DebugScene::UpdateTextEditor() {
    if (!textPreview_) { return; }

    // 文字/スタイルの変更は再生成（1フレームに1回だけ）
    if (textDirty_) {
        textPreview_->SetParams(BuildTextParams());
        textDirty_ = false;
    }

    // 位置・スケール・回転・乗算色は毎フレームの軽い反映
    textPreview_->SetAnchor({ 0.5f, 0.5f });
    textPreview_->SetPosition({ textPos_[0], textPos_[1] });
    textPreview_->SetScale(textScale_);
    textPreview_->SetRotation(textRotation_);
    textPreview_->SetColorMultiply({ textMul_[0], textMul_[1], textMul_[2], textMul_[3] });
}

void DebugScene::DrawTextPreview() {
    spCommon_->DrawCommonSetting();
    if (textPreview_) { textPreview_->Draw(); }
    if (textLoadedCheck_) { textLoadedCheck_->Draw(); }
}

void DebugScene::DrawTextUI() {
#ifdef _DEBUG
    EditorUI* editor = EditorUI::GetInstance();
    if (!editor->PanelVisible("テキストエディタ", "テキスト")) { return; }

    ImGui::Begin("Text Editor");

    // ---- 文字入力 ----
    if (ImGui::CollapsingHeader("文字", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::InputTextMultiline("##text", textBuf_, sizeof(textBuf_), ImVec2(-1.0f, 80.0f))) {
            textDirty_ = true;
        }
        ImGui::TextDisabled("改行可。日本語は IME で入力できます。");
    }

    // ---- フォント ----
    if (ImGui::CollapsingHeader("フォント", ImGuiTreeNodeFlags_DefaultOpen)) {
        std::string curName = std::filesystem::path(CurrentFontPath()).filename().string();
        if (ImGui::BeginCombo("フォント", curName.c_str())) {
            for (int i = 0; i < static_cast<int>(fontFiles_.size()); ++i) {
                std::string name = std::filesystem::path(fontFiles_[i]).filename().string();
                bool selected = (i == textFontIndex_);
                if (ImGui::Selectable(name.c_str(), selected)) {
                    textFontIndex_ = i;
                    textDirty_ = true;
                }
                if (selected) { ImGui::SetItemDefaultFocus(); }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("再スキャン")) { ScanFonts(); textDirty_ = true; }
        if (ImGui::Checkbox("日本語をフォールバック補完 (PixelMplus)", &textAutoFallback_)) { textDirty_ = true; }
    }

    // ---- スタイル ----
    if (ImGui::CollapsingHeader("スタイル", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderFloat("サイズ(px)", &textSize_, 8.0f, 300.0f, "%.0f")) { textDirty_ = true; }
        if (ImGui::ColorEdit4("塗り色", textColor_)) { textDirty_ = true; }

        if (ImGui::Checkbox("アウトライン", &textOutline_)) { textDirty_ = true; }
        if (textOutline_) {
            if (ImGui::ColorEdit4("アウトライン色", textOutlineColor_)) { textDirty_ = true; }
            if (ImGui::SliderFloat("アウトライン太さ(px)", &textOutlineWidth_, 0.0f, 20.0f, "%.1f")) { textDirty_ = true; }
        }

        const char* aligns[] = { "左", "中央", "右" };
        if (ImGui::Combo("揃え", &textAlign_, aligns, IM_ARRAYSIZE(aligns))) { textDirty_ = true; }
        if (ImGui::SliderFloat("行間", &textLineSpacing_, 0.5f, 2.5f, "%.2f")) { textDirty_ = true; }
    }

    // ---- シーン配置（再生成しない軽い操作） ----
    if (ImGui::CollapsingHeader("シーン配置", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat2("位置(px)", textPos_, 1.0f);
        ImGui::SliderFloat("スケール", &textScale_, 0.1f, 4.0f, "%.2f");
        ImGui::SliderFloat("回転(rad)", &textRotation_, -3.14159f, 3.14159f, "%.2f");
        ImGui::ColorEdit4("乗算色", textMul_);
        ImGui::TextDisabled("位置/スケール/回転/乗算色はプログラムからも同様に変更できます。");
    }

    // ---- プレビュー画像 ----
    if (textPreview_ && textPreview_->IsValid() &&
        TextureManager::GetInstance()->HasTexture(textPreview_->GetTextureKey())) {
        ImGui::Separator();
        ImGui::TextUnformatted("プレビュー");
        Vector2 tex = textPreview_->GetSprite()->GetTexSize();
        float w = tex.x, h = tex.y;
        const float maxW = 360.0f;
        if (w > maxW && w > 0.0f) { h *= maxW / w; w = maxW; }
        uint32_t idx = textPreview_->GetSrvIndex();
        ImTextureID tid = static_cast<ImTextureID>(SrvManager::GetInstance()->GetGPUDescriptorHandle(idx).ptr);
        ImGui::Image(tid, ImVec2(w, h));
        ImGui::TextDisabled("生成サイズ: %d x %d px", static_cast<int>(tex.x), static_cast<int>(tex.y));
    }

    // ---- 保存 / 読み込み ----
    if (ImGui::CollapsingHeader("保存 / 出力", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputText("ファイル名", textOutName_, sizeof(textOutName_));
        ImGui::TextDisabled("resources/images/ に PNG として保存されます（拡張子不要）。");

        if (ImGui::Button("PNG 保存")) {
            std::string name = textOutName_;
            // 末尾の .png は取り除く
            if (name.size() >= 4) {
                std::string tail = name.substr(name.size() - 4);
                std::transform(tail.begin(), tail.end(), tail.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (tail == ".png") { name = name.substr(0, name.size() - 4); }
            }
            if (name.empty()) { name = "myText"; }
            std::string outPath = "resources/images/" + name + ".png";
            if (TextTextureBaker::BakeToPng(BuildTextParams(), outPath)) {
                textLastSaved_ = outPath;
                std::string msg = outPath + " を保存しました。\nスプライトと同じ要領で読み込めます。";
                MessageBoxA(nullptr, msg.c_str(), "Text Editor", 0);
            } else {
                MessageBoxA(nullptr, "保存に失敗しました（文字が空か、フォント読込に失敗）。", "Text Editor", 0);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("保存画像をスプライトで確認")) {
            // 通常スプライトとして読み込み、シーンに表示（保存→読込の往復確認）。
            if (!textLastSaved_.empty()) {
                std::string fileName = std::filesystem::path(textLastSaved_).filename().string();
                textLoadedCheck_ = std::make_unique<Sprite>();
                textLoadedCheck_->Initialize(fileName, { textPos_[0], textPos_[1] + 180.0f }, { 1,1,1,1 }, { 0.5f, 0.5f });
            }
        }
        if (textLoadedCheck_) {
            ImGui::SameLine();
            if (ImGui::SmallButton("確認を消す")) { textLoadedCheck_.reset(); }
        }
        if (!textLastSaved_.empty()) {
            ImGui::TextDisabled("保存済み: %s", textLastSaved_.c_str());
        }
    }

    // ---- 使い方メモ ----
    if (ImGui::CollapsingHeader("プログラムからの使い方")) {
        ImGui::TextWrapped(
            "TextSprite label;\n"
            "TextRenderParams p; p.text = \"SCORE\"; p.pixelSize = 48; p.color = {1,1,0,1};\n"
            "label.Initialize(p, {100, 40});\n"
            "label.SetText(\"SCORE 120\");   // 文字を変更\n"
            "label.SetColorMultiply({1,0,0,1}); // 色を変更\n"
            "label.SetPosition({200, 40});  // 位置を変更\n"
            "label.Draw();\n\n"
            "保存済み PNG を使う場合:\n"
            "label.InitializeFromFile(\"myText.png\", {100, 40});");
    }

    ImGui::End();
#endif // _DEBUG
}
