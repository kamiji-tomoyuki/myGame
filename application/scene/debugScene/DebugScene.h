#pragma once
#include "BaseScene.h"
#include "ParticleEmitter.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "Input.h"
#include "Audio.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "SpriteCommon.h"
#include "ParticleCommon.h"
#include "DebugCamera.h"
#include "Skybox.h"
#include "PlayerMotionClip.h"
#include "PlayerComboMotion.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

/// <summary>
/// デバッグ用シーン（パーティクル仮作成＋プレイヤーモーション作成）
/// </summary>
using namespace Engine;
class DebugScene : public BaseScene {
public:
    void Initialize() override;
    void Finalize() override;
    void Update() override;
    void Draw() override;
    void DrawForOffScreen() override;

    ViewProjection* GetViewProjection() override { return &vp_; }

private:
    void Debug();

    // --- パーティクル ---
    void DrawParticleUI();

    // --- モーション ---
    void InitRig();
    void UpdateRig();               // pose_ をプレビューリグに反映
    void UpdateMotionPlayback();    // 再生時に時刻を進めてサンプリング
    void ApplyPoseFromClip(float t);// クリップの補間結果を pose_ へ
    void CaptureKeyframe();         // 現在の pose_ を時刻 editTime_ のキーに追加/更新
    void ScanClips();               // resources/jsons/motion を走査
    void AppendClipToCombo();       // 現在のクリップ名を combo.json 末尾へ登録
    std::vector<std::string> ReadComboClips() const; // combo.json のクリップ順を取得
    void WriteComboJson(const std::vector<std::string>& clips); // clips + 補間時間 を書き出す
    void StartComboPreview();       // combo.json のコンボを連続再生
    void SetCameraPreset(int preset); // 0=正面 1=背面 2=横 3=俯瞰
    void DrawMotionUI();
    void DrawRig();

private:
    Audio* audio_ = nullptr;
    Object3dCommon* objCommon_ = nullptr;
    SpriteCommon* spCommon_ = nullptr;
    ParticleCommon* ptCommon_ = nullptr;
    Input* input_ = nullptr;

    ViewProjection vp_;
    std::unique_ptr<DebugCamera> debugCamera_;
    std::unique_ptr<Skybox> skybox_;

    int editorMode_ = 1; // 0=パーティクル 1=モーション

    // --- パーティクル ---
    std::unique_ptr<ParticleEmitter> emitter_;
    char particleName_[64] = "new_particle";
    char modelPath_[128] = "debug/ringPlane.obj";
    int currentPrimitive_ = 0;

    // --- モーション プレビューリグ ---
    // 左右の腕はそれぞれ独立した Object3d（変換用CBが1つのため使い回すと同じ姿勢で重なる）
    std::unique_ptr<Object3d> bodyObj_;
    std::unique_ptr<Object3d> rArmObj_;
    std::unique_ptr<Object3d> lArmObj_;
    WorldTransform bodyTf_;
    WorldTransform rArmTf_;
    WorldTransform lArmTf_;

    // --- モーション編集状態 ---
    PlayerMotionClip clip_;
    std::array<PartPose, 3> pose_{}; // 0=体 1=右腕 2=左腕（現在表示姿勢）
    int   selectedPart_ = 1;         // 既定は右腕
    float editTime_ = 0.0f;
    bool  playing_ = false;
    bool  loop_ = true;
    char  clipName_[64] = "punchR";
    char  mirrorName_[64] = "punchL";
    std::vector<std::string> clipFiles_;

    // --- コンボ再生プレビュー ---
    std::unique_ptr<PlayerComboMotion> comboPreview_;
    bool  comboPlaying_ = false;
    bool  comboLoop_ = true;
    float comboBlendTime_ = 0.1f; // クリップ間の補間時間（秒）

    // リグ基準値（実プレイヤーと同値）
    const Vector3 armScale_ = { 0.8f, 0.8f, 0.8f };
    const Vector3 rArmBase_ = { 1.7f, 0.0f, 1.3f };
    const Vector3 lArmBase_ = { -1.7f, 0.0f, 1.3f };
};
