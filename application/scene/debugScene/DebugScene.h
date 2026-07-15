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
#include "PlayerArmRush.h"
#include "ObjColor.h"
#include "TextSprite.h"
#include "Sprite.h"
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

    // --- ラッシュプレビュー ---
    void InitRushPreview();
    void StartRushPreview();
    void UpdateRushPreview();
    void DrawRushPreview();
    void DrawRushUI();

    // --- テキストエディタ ---
    void ScanFonts();                       // resources/fonts を走査
    void InitTextEditor();                  // プレビュー生成
    void UpdateTextEditor();                // UI 変更をプレビューへ反映
    void DrawTextPreview();                 // シーン内へテキストを仮描画
    void DrawTextUI();                      // ImGui エディタ
    TextRenderParams BuildTextParams() const;
    std::string CurrentFontPath() const;    // 選択中フォントのパス
    std::string FallbackFontPath() const;   // 日本語補完フォントのパス

private:
    Audio* audio_ = nullptr;
    Object3dCommon* objCommon_ = nullptr;
    SpriteCommon* spCommon_ = nullptr;
    ParticleCommon* ptCommon_ = nullptr;
    Input* input_ = nullptr;

    ViewProjection vp_;
    std::unique_ptr<DebugCamera> debugCamera_;
    std::unique_ptr<Skybox> skybox_;

    int editorMode_ = 1; // 0=パーティクル 1=モーション 2=ラッシュ 3=テキスト

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
    // オートで連鎖するか。オフにすると「コンボ入力」ボタンで手動連鎖し、
    // 各クリップの「コンボ受付開始」以降でのみ次段へ繋がる＝受付時間の効果を確認できる。
    bool  comboAutoAdvance_ = false;
    float comboBlendTime_ = 0.1f; // クリップ間の補間時間（秒）

    // --- ラッシュプレビュー ---
    static constexpr int kRushTrail_ = 3; // 片側の残像本数
    std::unique_ptr<PlayerArmRush> rushR_;
    std::unique_ptr<PlayerArmRush> rushL_;
    std::array<std::unique_ptr<PlayerArmRush>, kRushTrail_> trailRushR_;
    std::array<std::unique_ptr<PlayerArmRush>, kRushTrail_> trailRushL_;
    std::array<std::unique_ptr<Object3d>, kRushTrail_> trailArmR_;
    std::array<std::unique_ptr<Object3d>, kRushTrail_> trailArmL_;
    std::array<WorldTransform, kRushTrail_> trailTfR_;
    std::array<WorldTransform, kRushTrail_> trailTfL_;
    std::unique_ptr<PlayerComboMotion> finisherPreview_;
    bool  rushPlaying_ = false;
    bool  rushLoop_ = true;
    bool  rushFinStarted_ = false;
    float rushTrailAlpha_ = 0.5f;
    float rushTrailFalloff_ = 0.6f;
    int   rushTrailStep_ = 3;

    // リグ基準値（実プレイヤーと同値）
    const Vector3 armScale_ = { 0.8f, 0.8f, 0.8f };
    const Vector3 rArmBase_ = { 1.7f, 0.0f, 1.3f };
    const Vector3 lArmBase_ = { -1.7f, 0.0f, 1.3f };

    // --- テキストエディタ状態 ---
    std::unique_ptr<TextSprite> textPreview_;            // シーン内の仮描画スプライト
    std::unique_ptr<Sprite>     textLoadedCheck_;        // 保存後の読み込み確認用
    std::vector<std::string>    fontFiles_;              // resources/fonts の .ttf 一覧
    int   textFontIndex_ = 0;                            // 選択中フォント
    bool  textAutoFallback_ = true;                      // 日本語をフォールバックフォントで補完
    char  textBuf_[1024] = "Text\nテキスト";             // 入力文字（UTF-8）
    char  textOutName_[64] = "myText";                   // 保存ファイル名（拡張子なし）
    float textSize_ = 96.0f;                             // 文字サイズ(px)
    float textColor_[4] = { 1.0f, 0.85f, 0.2f, 1.0f };   // 塗り色
    bool  textOutline_ = true;                           // アウトライン有無
    float textOutlineColor_[4] = { 0.12f, 0.10f, 0.08f, 1.0f };
    float textOutlineWidth_ = 5.0f;                      // アウトライン太さ(px)
    int   textAlign_ = 1;                                // 0=左 1=中央 2=右
    float textLineSpacing_ = 1.0f;                       // 行間倍率
    float textPos_[2] = { 320.0f, 220.0f };              // 仮描画の画面位置(px)
    float textScale_ = 1.0f;                             // 表示スケール
    float textRotation_ = 0.0f;                          // 回転(rad)
    float textMul_[4] = { 1.0f, 1.0f, 1.0f, 1.0f };      // スプライト乗算色
    bool  textDirty_ = true;                             // 再生成が必要
    std::string textLastSaved_;                          // 直近保存パス
};
