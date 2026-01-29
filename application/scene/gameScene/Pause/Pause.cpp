#include "Pause.h"
#include <cmath>

void Pause::Initialize()
{
	input_ = Input::GetInstance();

	// ===== ポーズ画面UI初期化 =====
	// 背景（半透明の黒）
	pauseBackground_ = std::make_unique<Sprite>();
	pauseBackground_->Initialize("white1x1.png", { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.7f }, { 0.0f, 0.0f }); // 左上基準
	pauseBackground_->SetSize({ 1280.0f, 720.0f }); // 画面全体

	// PAUSEロゴ（画面中央上部）
	pauseTitle_ = std::make_unique<Sprite>();
	pauseTitle_->Initialize("pauseTitle.png", { 100.0f, 60.0f });

	// 再生マーク（PAUSEロゴと同じ位置）
	pausePlayIcon_ = std::make_unique<Sprite>();
	pausePlayIcon_->Initialize("pausePlay.png", { 100.0f, 60.0f });
	pausePlayIcon_->SetAlpha(0.0f); // 初期状態では非表示

	// メニュー項目（画面中央）
	pauseResume_ = std::make_unique<Sprite>();
	pauseResume_->Initialize("pauseResume.png", { 640.0f, 300.0f });
	pauseResume_->SetAnchorPoint({ 0.5f, 0.5f });

	pauseRetry_ = std::make_unique<Sprite>();
	pauseRetry_->Initialize("pauseRetry.png", { 640.0f, 380.0f });
	pauseRetry_->SetAnchorPoint({ 0.5f, 0.5f });

	pauseToTitle_ = std::make_unique<Sprite>();
	pauseToTitle_->Initialize("pauseToTitle.png", { 640.0f, 460.0f });
	pauseToTitle_->SetAnchorPoint({ 0.5f, 0.5f });

	// 矢印スプライト初期化
	pauseArrowUp_ = std::make_unique<Sprite>();
	pauseArrowUp_->Initialize("ArrowUp.png", { 640.0f, 200.0f });
	pauseArrowUp_->SetAnchorPoint({ 0.5f, 0.5f });

	pauseArrowDown_ = std::make_unique<Sprite>();
	pauseArrowDown_->Initialize("ArrowDown.png", { 640.0f, 300.0f });
	pauseArrowDown_->SetAnchorPoint({ 0.5f, 0.5f });

	// SPACEロゴ
	pauseSPACE_ = std::make_unique<Sprite>();
	pauseSPACE_->Initialize("pauseSPACE.png", { 1180.0f, 660.0f });
	pauseSPACE_->SetAnchorPoint({ 1.0f, 1.0f });

	// ポーズ状態初期化
	isPaused_ = false;
	currentSelection_ = PauseMenuSelection::Resume;
	pauseAnimationTimer_ = 0.0f;
	isPlayIconFading_ = false;
	playIconAlpha_ = 0.0f;
	spaceBlinkTimer_ = 0.0f;
	isUIPauseFadingIn_ = false;
	uiPauseAlpha_ = 1.0f;

	// 補間用変数の初期化
	resumeCurrentY_ = kResumeBaseY_;
	retryCurrentY_ = kRetryBaseY_;
	toTitleCurrentY_ = kToTitleBaseY_;

	// リクエストフラグ初期化
	requestRetry_ = false;
	requestBackToTitle_ = false;
}

void Pause::Update()
{
	if (!isPaused_) {
		return;
	}

	// アニメーションタイマーを更新
	pauseAnimationTimer_ += kAnimationSpeed_ / 60.0f; // 60FPS想定

	// SPACEロゴの点滅タイマーを更新
	spaceBlinkTimer_ += kSpaceBlinkSpeed_ / 60.0f;

	// 上キー入力チェック（WキーまたはUPキー）
	bool currentUpKeyState = input_->PushKey(DIK_UP) || input_->PushKey(DIK_W);
	if (currentUpKeyState && !prevUpKeyState_) {
		int selection = static_cast<int>(currentSelection_);
		selection--;
		if (selection < 0) {
			selection = static_cast<int>(PauseMenuSelection::MaxCount) - 1;
		}
		currentSelection_ = static_cast<PauseMenuSelection>(selection);
	}
	prevUpKeyState_ = currentUpKeyState;

	// 下キー入力チェック（SキーまたはDOWNキー）
	bool currentDownKeyState = input_->PushKey(DIK_DOWN) || input_->PushKey(DIK_S);
	if (currentDownKeyState && !prevDownKeyState_) {
		int selection = static_cast<int>(currentSelection_);
		selection++;
		if (selection >= static_cast<int>(PauseMenuSelection::MaxCount)) {
			selection = 0;
		}
		currentSelection_ = static_cast<PauseMenuSelection>(selection);
	}
	prevDownKeyState_ = currentDownKeyState;

	// 決定キー（Spaceキー）入力チェック
	bool currentSpaceKeyState = input_->TriggerKey(DIK_SPACE);
	if (currentSpaceKeyState && !prevSpaceKeyState_) {
		ExecutePauseSelection();
	}
	prevSpaceKeyState_ = currentSpaceKeyState;

	// メニュー項目の位置を計算
	CalculateMenuPositions();
}

void Pause::Draw()
{
	if (!isPaused_) {
		return;
	}

	// 背景（半透明）
	pauseBackground_->Draw();

	// PAUSEロゴ(停止マーク) - ポーズ中のみ表示
	pauseTitle_->Draw();

	// SPACEロゴ - sin波で点滅
	float spaceAlpha = (std::sin(spaceBlinkTimer_) * 0.5f + 0.5f) * 0.7f + 0.3f; // 0.3～1.0の範囲で点滅
	pauseSPACE_->SetAlpha(spaceAlpha);
	pauseSPACE_->Draw();

	// sin波を使ってふわふわ動く矢印のオフセットを計算
	float arrowOffset = std::sin(pauseAnimationTimer_) * kArrowFloatRange_;

	// 選択中の項目のY座標を取得
	float selectedY = 0.0f;
	switch (currentSelection_) {
	case PauseMenuSelection::Resume:
		selectedY = kResumeBaseY_;
		break;
	case PauseMenuSelection::Retry:
		selectedY = kRetryBaseY_;
		break;
	case PauseMenuSelection::BackToTitle:
		selectedY = kToTitleBaseY_;
		break;
	}

	// メニュー項目の描画
	// --- 再開 ---
	if (currentSelection_ != PauseMenuSelection::Resume) {
		pauseResume_->SetColor({ 0.5f, 0.5f, 0.5f });
		pauseResume_->SetAlpha(0.6f);
	}
	else {
		pauseResume_->SetColor({ 1.0f, 1.0f, 1.0f });
		pauseResume_->SetAlpha(1.0f);
	}
	pauseResume_->SetPosition({ 640.0f, resumeCurrentY_ });
	pauseResume_->Draw();

	// --- 再挑戦 ---
	if (currentSelection_ != PauseMenuSelection::Retry) {
		pauseRetry_->SetColor({ 0.5f, 0.5f, 0.5f });
		pauseRetry_->SetAlpha(0.6f);
	}
	else {
		pauseRetry_->SetColor({ 1.0f, 1.0f, 1.0f });
		pauseRetry_->SetAlpha(1.0f);
	}
	pauseRetry_->SetPosition({ 640.0f, retryCurrentY_ });
	pauseRetry_->Draw();

	// --- タイトルへ戻る ---
	if (currentSelection_ != PauseMenuSelection::BackToTitle) {
		pauseToTitle_->SetColor({ 0.5f, 0.5f, 0.5f });
		pauseToTitle_->SetAlpha(0.6f);
	}
	else {
		pauseToTitle_->SetColor({ 1.0f, 1.0f, 1.0f });
		pauseToTitle_->SetAlpha(1.0f);
	}
	pauseToTitle_->SetPosition({ 640.0f, toTitleCurrentY_ });
	pauseToTitle_->Draw();

	// --- 矢印の描画（選択中の項目の上下） ---
	const float arrowX = 640.0f; // 画面中央
	const float arrowSpacing = 80.0f; // 選択項目から矢印までの距離

	// 上矢印
	pauseArrowUp_->SetPosition({ arrowX, selectedY - arrowSpacing + arrowOffset });
	pauseArrowUp_->Draw();

	// 下矢印
	pauseArrowDown_->SetPosition({ arrowX, selectedY + arrowSpacing - arrowOffset });
	pauseArrowDown_->Draw();
}

void Pause::TogglePause()
{
	isPaused_ = !isPaused_;

	if (isPaused_) {
		// ポーズ開始時の処理
		currentSelection_ = PauseMenuSelection::Resume;

		// 補間用変数を初期位置にリセット
		resumeCurrentY_ = kResumeBaseY_;
		retryCurrentY_ = kRetryBaseY_;
		toTitleCurrentY_ = kToTitleBaseY_;

		// 再生マークフェードアウトを停止
		isPlayIconFading_ = false;
		playIconAlpha_ = 0.0f;
		pausePlayIcon_->SetAlpha(0.0f);

		// UIPauseを非表示に（ポーズ中は表示しない）
		isUIPauseFadingIn_ = false;
		uiPauseAlpha_ = 0.0f;
	}
	else {
		// ポーズ解除時の処理（再開を選択した場合）
		// 再生マークのフェードアウトを開始
		isPlayIconFading_ = true;
		playIconAlpha_ = 1.0f;
		pausePlayIcon_->SetAlpha(1.0f);

		// UIPauseは再生マークが消えてからフェードイン開始（Update関数で制御）
		isUIPauseFadingIn_ = false;
		uiPauseAlpha_ = 0.0f;
	}
}

void Pause::ExecutePauseSelection()
{
	switch (currentSelection_) {
	case PauseMenuSelection::Resume:
		// 再開
		TogglePause();
		break;

	case PauseMenuSelection::Retry:
		// 再挑戦 - フラグを立てる
		requestRetry_ = true;
		isPaused_ = false; // ポーズ解除
		break;

	case PauseMenuSelection::BackToTitle:
		// タイトルへ戻る - フラグを立てる
		requestBackToTitle_ = true;
		isPaused_ = false; // ポーズ解除
		break;
	}
}

void Pause::CalculateMenuPositions()
{
	// 選択中の項目のY座標を取得
	float selectedY = 0.0f;
	switch (currentSelection_) {
	case PauseMenuSelection::Resume:
		selectedY = kResumeBaseY_;
		break;
	case PauseMenuSelection::Retry:
		selectedY = kRetryBaseY_;
		break;
	case PauseMenuSelection::BackToTitle:
		selectedY = kToTitleBaseY_;
		break;
	}

	// --- 目標Y座標の計算 ---
	float resumeTargetY = kResumeBaseY_;
	float retryTargetY = kRetryBaseY_;
	float toTitleTargetY = kToTitleBaseY_;

	// 選択されていない項目は、選択中の項目から離れる
	if (currentSelection_ != PauseMenuSelection::Resume) {
		if (kResumeBaseY_ < selectedY) {
			resumeTargetY -= kPauseItemOffset_; // 上に移動
		}
		else {
			resumeTargetY += kPauseItemOffset_; // 下に移動
		}
	}

	if (currentSelection_ != PauseMenuSelection::Retry) {
		if (kRetryBaseY_ < selectedY) {
			retryTargetY -= kPauseItemOffset_; // 上に移動
		}
		else {
			retryTargetY += kPauseItemOffset_; // 下に移動
		}
	}

	if (currentSelection_ != PauseMenuSelection::BackToTitle) {
		if (kToTitleBaseY_ < selectedY) {
			toTitleTargetY -= kPauseItemOffset_; // 上に移動
		}
		else {
			toTitleTargetY += kPauseItemOffset_; // 下に移動
		}
	}

	// --- 補間アニメーション（線形補間） ---
	resumeCurrentY_ += (resumeTargetY - resumeCurrentY_) * lerpSpeed_;
	retryCurrentY_ += (retryTargetY - retryCurrentY_) * lerpSpeed_;
	toTitleCurrentY_ += (toTitleTargetY - toTitleCurrentY_) * lerpSpeed_;
}

void Pause::UpdatePlayIconFade()
{
	if (!isPlayIconFading_) {
		return;
	}

	playIconAlpha_ -= kPlayIconFadeSpeed_;
	if (playIconAlpha_ <= 0.0f) {
		playIconAlpha_ = 0.0f;
		isPlayIconFading_ = false;
		// 再生マークが完全に消えたらUIPauseのフェードインを開始
		isUIPauseFadingIn_ = true;
	}
	pausePlayIcon_->SetAlpha(playIconAlpha_);
}

void Pause::UpdateUIPauseFade()
{
	if (!isUIPauseFadingIn_) {
		return;
	}

	uiPauseAlpha_ += kUIPauseFadeSpeed_;
	if (uiPauseAlpha_ >= 1.0f) {
		uiPauseAlpha_ = 1.0f;
		isUIPauseFadingIn_ = false;
	}
}

void Pause::DrawPlayIcon()
{
	if (isPlayIconFading_ && playIconAlpha_ > 0.0f) {
		pausePlayIcon_->Draw();
	}
}