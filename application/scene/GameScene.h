#pragma once
#include "Audio.h"
#include "BaseScene.h"
#include "DebugCamera.h"
#include "Input.h"
#include "Object3dCommon.h"
#include "ParticleCommon.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "ViewProjection.h"

#include <Player.h>
#include <Enemy.h>
#include <FollowCamera.h>
#include <Ground.h>

#ifdef _DEBUG
#include "temp/TempObj.h"
#endif //_DEBUG
#include <ParticleEmitter.h>
#include <Skybox.h>

enum class GamePhase {
	EnemyAppear,  // 敵出現演出
	Battle,       // 戦闘パート
};

// ポーズメニューの選択項目
enum class PauseMenuSelection {
	Resume,      // 再開
	Retry,       // 再挑戦
	BackToTitle, // タイトルへ戻る
	MaxCount     // 項目数
};

class GameScene : public BaseScene
{
public: // メンバ関数

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize()override;

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize()override;

	/// <summary>
	/// 更新
	/// </summary>
	void Update()override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw()override;

	/// <summary>
	/// オフスクリーン上に描画
	/// </summary>
	void DrawForOffScreen()override;

	ViewProjection* GetViewProjection()override { return &vp_; }
private:
	/// <summary>
	/// デバック
	/// </summary>
	void Debug();

	/// <summary>
	/// カメラの更新
	/// </summary>
	void CameraUpdate();

	void UpdateStart();
	void UpdateBattle();

	void ChangeScene();

	/// <summary>
	/// 攻撃UIの更新
	/// </summary>
	void UpdateAttackUI();

	/// <summary>
	/// ポーズ画面の更新
	/// </summary>
	void UpdatePause();

	/// <summary>
	/// ポーズ画面の描画
	/// </summary>
	void DrawPause();

	/// <summary>
	/// ポーズ状態の切り替え
	/// </summary>
	void TogglePause();

	/// <summary>
	/// ポーズメニューの選択処理
	/// </summary>
	void ExecutePauseSelection();

private:

	Audio* audio_;
	Input* input_;
	Object3dCommon* objCommon_;
	SpriteCommon* spCommon_;
	ParticleCommon* ptCommon_;

	GamePhase currentPhase_;

	// ビュープロジェクション
	ViewProjection vp_;
	std::unique_ptr<DebugCamera> debugCamera_;

	// --- 各オブジェクト ---
	std::unique_ptr<Player> player_;
	std::unique_ptr<Enemy> enemy_;

	std::unique_ptr<Skybox> skybox_;
	std::unique_ptr<Ground> ground_;

	std::unique_ptr<FollowCamera> followCamera_;

	// --- 各エフェクト・演出 ---
	std::unique_ptr<ParticleEmitter> stageWall_;

	bool isStart = false;
	bool isCameraMoveStart_ = false;

	// --- シーン管理 ---
	bool isClear = false;

	// --- スプライト ---
	std::unique_ptr<Sprite> UI_;
	std::unique_ptr<Sprite> attackUI_Right_;   // 右フック用UI
	std::unique_ptr<Sprite> attackUI_Left_;    // 左フック用UI
	std::unique_ptr<Sprite> attackUI_Rush_;    // ラッシュ用UI
	std::unique_ptr<Sprite> UIPause_;          // ポーズ表示用UI

	// --- ポーズ画面用 ---
	bool isPaused_ = false;                    // ポーズ中かどうか
	PauseMenuSelection currentSelection_ = PauseMenuSelection::Resume; // 現在の選択項目

	std::unique_ptr<Sprite> pauseBackground_;  // ポーズ画面背景（半透明）
	std::unique_ptr<Sprite> pauseTitle_;       // "PAUSE"テキスト(停止マーク)
	std::unique_ptr<Sprite> pausePlayIcon_;    // 再生マーク
	std::unique_ptr<Sprite> pauseResume_;      // "再開"選択肢
	std::unique_ptr<Sprite> pauseRetry_;       // "再挑戦"選択肢
	std::unique_ptr<Sprite> pauseToTitle_;     // "タイトルへ戻る"選択肢
	std::unique_ptr<Sprite> pauseSPACE_;		// "SPACE"

	// 矢印スプライト
	std::unique_ptr<Sprite> pauseArrowUp_;     // 上矢印
	std::unique_ptr<Sprite> pauseArrowDown_;   // 下矢印

	bool prevPauseKeyState_ = false;           // 前フレームのポーズキー状態（連続入力防止）
	bool prevUpKeyState_ = false;              // 前フレームの上キー状態
	bool prevDownKeyState_ = false;            // 前フレームの下キー状態
	bool prevSpaceKeyState_ = false;           // 前フレームのSpaceキー状態

	// ポーズ演出用
	float pauseAnimationTimer_ = 0.0f;         // 演出用タイマー
	const float kPauseItemOffset_ = 50.0f;     // 非選択項目のオフセット距離
	const float kArrowFloatRange_ = 15.0f;     // 矢印の上下移動範囲
	const float kAnimationSpeed_ = 2.0f;       // アニメーション速度

	// 再生マークフェードアウト用
	bool isPlayIconFading_ = false;            // 再生マークフェード中かどうか
	float playIconAlpha_ = 0.0f;               // 再生マークのアルファ値
	const float kPlayIconFadeSpeed_ = 0.03f;   // 再生マークのフェード速度

	// UIPauseフェードイン用
	bool isUIPauseFadingIn_ = false;           // UIPauseフェードイン中かどうか
	float uiPauseAlpha_ = 0.0f;                // UIPauseのアルファ値
	const float kUIPauseFadeSpeed_ = 0.03f;    // UIPauseのフェード速度

	// SPACEロゴの点滅用
	float spaceBlinkTimer_ = 0.0f;             // 点滅用タイマー
	const float kSpaceBlinkSpeed_ = 3.0f;      // 点滅速度

	// 各メニュー項目の初期Y座標
	const float kResumeBaseY_ = 260.0f;
	const float kRetryBaseY_ = 380.0f;
	const float kToTitleBaseY_ = 500.0f;

	// 補間アニメーション用
	float resumeCurrentY_ = 260.0f;            // 再開の現在Y座標
	float retryCurrentY_ = 380.0f;             // 再挑戦の現在Y座標
	float toTitleCurrentY_ = 500.0f;           // タイトルへ戻るの現在Y座標
	float lerpSpeed_ = 0.15f;                  // 補間速度（0.0～1.0、大きいほど速い）

#ifdef _DEBUG
	std::unique_ptr<TempObj> obj_;
#endif // _DEBUG
};