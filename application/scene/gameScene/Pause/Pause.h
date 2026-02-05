#pragma once
#include "Sprite.h"
#include "Input.h"
#include <memory>

// ポーズメニューの選択項目
enum class PauseMenuSelection {
	Resume,      // 再開
	Retry,       // 再挑戦
	BackToTitle, // タイトルへ戻る
	MaxCount     // 項目数
};

/// <summary>
/// ポーズ画面管理クラス
/// </summary>
class Pause {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// ポーズ状態の切り替え
	/// </summary>
	void TogglePause();

	/// <summary>
	/// ポーズ中かどうかを取得
	/// </summary>
	/// <returns>ポーズ中ならtrue</returns>
	bool IsPaused() const { return isPaused_; }

	/// <summary>
	/// 再生マークがフェードアウト中かどうかを取得
	/// </summary>
	bool IsPlayIconFading() const { return isPlayIconFading_; }

	/// <summary>
	/// 再生マークのアルファ値を取得
	/// </summary>
	float GetPlayIconAlpha() const { return playIconAlpha_; }

	/// <summary>
	/// UIPauseがフェードイン中かどうかを取得
	/// </summary>
	bool IsUIPauseFadingIn() const { return isUIPauseFadingIn_; }

	/// <summary>
	/// UIPauseのアルファ値を取得
	/// </summary>
	float GetUIPauseAlpha() const { return uiPauseAlpha_; }

	/// <summary>
	/// 再生マークのフェードアウトを更新
	/// </summary>
	void UpdatePlayIconFade();

	/// <summary>
	/// UIPauseのフェードインを更新
	/// </summary>
	void UpdateUIPauseFade();

	/// <summary>
	/// 再生マークの描画
	/// </summary>
	void DrawPlayIcon();

	/// <summary>
	/// 選択されたメニュー項目を取得
	/// </summary>
	PauseMenuSelection GetSelectedMenu() const { return currentSelection_; }

private:
	/// <summary>
	/// ポーズメニューの選択処理
	/// </summary>
	void ExecutePauseSelection();

	/// <summary>
	/// 各メニュー項目のY座標を計算
	/// </summary>
	void CalculateMenuPositions();

	Input* input_ = nullptr;

	// ポーズ状態
	bool isPaused_ = false;
	PauseMenuSelection currentSelection_ = PauseMenuSelection::Resume;

	// スプライト
	std::unique_ptr<Sprite> pauseBackground_;  // ポーズ画面背景（半透明）
	std::unique_ptr<Sprite> pauseTitle_;       // "PAUSE"テキスト(停止マーク)
	std::unique_ptr<Sprite> pausePlayIcon_;    // 再生マーク
	std::unique_ptr<Sprite> pauseResume_;      // "再開"選択肢
	std::unique_ptr<Sprite> pauseRetry_;       // "再挑戦"選択肢
	std::unique_ptr<Sprite> pauseToTitle_;     // "タイトルへ戻る"選択肢
	std::unique_ptr<Sprite> pauseSPACE_;       // "SPACE"

	// 矢印スプライト
	std::unique_ptr<Sprite> pauseArrowUp_;     // 上矢印
	std::unique_ptr<Sprite> pauseArrowDown_;   // 下矢印

	// 入力状態（連続入力防止用）
	bool prevUpKeyState_ = false;
	bool prevDownKeyState_ = false;
	bool prevSpaceKeyState_ = false;

	// ポーズ演出用
	float pauseAnimationTimer_ = 0.0f;
	const float kPauseItemOffset_ = 50.0f;
	const float kArrowFloatRange_ = 15.0f;
	const float kAnimationSpeed_ = 2.0f;

	// 再生マークフェードアウト用
	bool isPlayIconFading_ = false;
	float playIconAlpha_ = 0.0f;
	const float kPlayIconFadeSpeed_ = 0.03f;

	// UIPauseフェードイン用
	bool isUIPauseFadingIn_ = false;
	float uiPauseAlpha_ = 0.0f;
	const float kUIPauseFadeSpeed_ = 0.03f;

	// SPACEロゴの点滅用
	float spaceBlinkTimer_ = 0.0f;
	const float kSpaceBlinkSpeed_ = 3.0f;

	// 各メニュー項目の初期Y座標
	const float kResumeBaseY_ = 260.0f;
	const float kRetryBaseY_ = 380.0f;
	const float kToTitleBaseY_ = 500.0f;

	// 補間アニメーション用
	float resumeCurrentY_ = 260.0f;
	float retryCurrentY_ = 380.0f;
	float toTitleCurrentY_ = 500.0f;
	float lerpSpeed_ = 0.15f;

	// シーン切り替え要求フラグ
	bool requestRetry_ = false;
	bool requestBackToTitle_ = false;

public:
	/// <summary>
	/// リトライが要求されたかどうか
	/// </summary>
	bool IsRetryRequested() const { return requestRetry_; }

	/// <summary>
	/// タイトルへ戻ることが要求されたかどうか
	/// </summary>
	bool IsBackToTitleRequested() const { return requestBackToTitle_; }

	/// <summary>
	/// リクエストフラグをリセット
	/// </summary>
	void ResetRequests() {
		requestRetry_ = false;
		requestBackToTitle_ = false;
	}
};