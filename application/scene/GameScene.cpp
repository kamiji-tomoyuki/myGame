#include "GameScene.h"
#include "SceneManager.h"
#include "Easing.h"
#include <cmath>

#include <LightGroup.h>
#include <line/DrawLine3D.h>

void GameScene::Initialize()
{
	audio_ = Audio::GetInstance();
	objCommon_ = Object3dCommon::GetInstance();
	spCommon_ = SpriteCommon::GetInstance();
	ptCommon_ = ParticleCommon::GetInstance();
	input_ = Input::GetInstance();

	vp_.Initialize();

	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize(&vp_);

	// ===== 各オブジェクトの初期化 =====

	currentPhase_ = GamePhase::EnemyAppear;

	// --- プレイヤー ---
	Player::SetSerialNumber(0);
	player_ = std::make_unique<Player>();
	player_->Init();
	player_->SetViewProjection(&vp_);
	player_->SetGameState(Player::GameState::kPlaying);

	// --- カメラ ---
	followCamera_ = std::make_unique<FollowCamera>();
	followCamera_->Initialize();
	followCamera_->SetPosition({ 0.0f,3.0f,9.0f });

	// --- 敵 ---
	Enemy::SetSerialNumber(0);
	enemy_ = std::make_unique<Enemy>();
	enemy_->Init();
	enemy_->SetGameState(Enemy::GameState::kPlaying);

	player_->SetEnemy(enemy_.get());

	// --- ステージ ---
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize("skybox.dds");

	ground_ = std::make_unique<Ground>();
	ground_->Init(skybox_.get());

	// ===== 各エフェクト・演出の初期化 =====
	stageWall_ = std::make_unique<ParticleEmitter>();
	stageWall_->Initialize("stage", "debug/ringPlane.obj");

	// ===== スプライト =====
	UI_ = std::make_unique<Sprite>();
	UI_->Initialize("gameUI.png", { 40.0f, 530.0f });

	// 攻撃UI初期化
	attackUI_Right_ = std::make_unique<Sprite>();
	attackUI_Right_->Initialize("gameUIRight.png", { 40.0f, 530.0f });

	attackUI_Left_ = std::make_unique<Sprite>();
	attackUI_Left_->Initialize("gameUILeft.png", { 40.0f, 530.0f });

	attackUI_Rush_ = std::make_unique<Sprite>();
	attackUI_Rush_->Initialize("gameUIRush.png", { 40.0f, 530.0f });

	// ポーズ表示用UI初期化（再生マークと同じ座標）
	UIPause_ = std::make_unique<Sprite>();
	UIPause_->Initialize("gameUIPause.png", { 40.0f, 60.0f });
	UIPause_->SetAlpha(1.0f); // 初期状態から表示

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
	pauseResume_->SetAnchorPoint({ 0.5f,0.5f });

	pauseRetry_ = std::make_unique<Sprite>();
	pauseRetry_->Initialize("pauseRetry.png", { 640.0f, 380.0f });
	pauseRetry_->SetAnchorPoint({ 0.5f,0.5f });

	pauseToTitle_ = std::make_unique<Sprite>();
	pauseToTitle_->Initialize("pauseToTitle.png", { 640.0f, 460.0f });
	pauseToTitle_->SetAnchorPoint({ 0.5f,0.5f });

	// 矢印スプライト初期化
	pauseArrowUp_ = std::make_unique<Sprite>();
	pauseArrowUp_->Initialize("ArrowUp.png", { 640.0f, 200.0f });
	pauseArrowUp_->SetAnchorPoint({ 0.5f,0.5f });

	pauseArrowDown_ = std::make_unique<Sprite>();
	pauseArrowDown_->Initialize("ArrowDown.png", { 640.0f, 300.0f });
	pauseArrowDown_->SetAnchorPoint({ 0.5f,0.5f });

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

#ifdef _DEBUG
	obj_ = std::make_unique<TempObj>();
	obj_->Init();
#endif // _DEBUG
}

void GameScene::Finalize()
{

}

void GameScene::Update()
{
	// ポーズキー（ESCまたはP）のチェック
	bool currentPauseKeyState = input_->TriggerKey(DIK_ESCAPE) || input_->TriggerKey(DIK_P);

	// ポーズキーが押された瞬間（前フレームは押されていなくて今フレーム押された）
	if (currentPauseKeyState && !prevPauseKeyState_ && currentPhase_ == GamePhase::Battle) {
		TogglePause();
	}
	prevPauseKeyState_ = currentPauseKeyState;

	// --- カメラ更新（ポーズ中でも実行） ---
	CameraUpdate();

	// --- フィールド更新（ポーズ中でも実行して画面に表示） ---
	skybox_->Update(vp_);
	ground_->Update();

	// ポーズ中の処理
	if (isPaused_) {
		UpdatePause();
		return; // ポーズ中はゲームロジックの更新を行わない
	}

	// 再生マークのフェードアウト処理（ポーズ解除後）
	if (isPlayIconFading_) {
		playIconAlpha_ -= kPlayIconFadeSpeed_;
		if (playIconAlpha_ <= 0.0f) {
			playIconAlpha_ = 0.0f;
			isPlayIconFading_ = false;
			// 再生マークが完全に消えたらUIPauseのフェードインを開始
			isUIPauseFadingIn_ = true;
		}
		pausePlayIcon_->SetAlpha(playIconAlpha_);
	}

	// UIPauseのフェードイン処理（再生マークが消えた後）
	if (isUIPauseFadingIn_) {
		uiPauseAlpha_ += kUIPauseFadeSpeed_;
		if (uiPauseAlpha_ >= 1.0f) {
			uiPauseAlpha_ = 1.0f;
			isUIPauseFadingIn_ = false;
		}
		UIPause_->SetAlpha(uiPauseAlpha_);
	}

#ifdef _DEBUG
	// デバッグ
	Debug();
#endif // _DEBUG

	// ===== 各オブジェクトの更新 =====
	switch (currentPhase_) {
	case GamePhase::EnemyAppear:
		UpdateStart();
		break;

	case GamePhase::Battle:
		UpdateBattle();
		UpdateAttackUI();  // 攻撃UIの更新
		break;
	}

	// ===== 各エフェクト・演出の更新 =====
	stageWall_->Update(vp_);

	// ===== シーン切り替え =====
	ChangeScene();

#ifdef _DEBUG
	// --- Debug時処理 ---
	obj_->Update();
#endif // _DEBUG

}

void GameScene::UpdateAttackUI()
{
	// プレイヤーが存在しない場合は何もしない
	if (!player_) {
		return;
	}

	// プレイヤーの腕の状態を取得
	// 右腕と左腕の情報を取得する必要があるため、Playerクラスに
	// 腕の状態を取得するメソッドが必要です
}

void GameScene::UpdatePause()
{
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
}

void GameScene::TogglePause()
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
		UIPause_->SetAlpha(0.0f);
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
		UIPause_->SetAlpha(0.0f);
	}
}

void GameScene::ExecutePauseSelection()
{
	switch (currentSelection_) {
	case PauseMenuSelection::Resume:
		// 再開
		TogglePause();
		break;

	case PauseMenuSelection::Retry:
		// 再挑戦 - 現在のシーンをリロード
		sceneManager_->NextSceneReservation("GAME");
		break;

	case PauseMenuSelection::BackToTitle:
		// タイトルへ戻る
		sceneManager_->NextSceneReservation("TITLE");
		break;
	}
}

void GameScene::Draw()
{
	/// -------描画処理開始-------

	skybox_->Draw();

	/// Spriteの描画準備
	spCommon_->DrawCommonSetting();
	//-----Spriteの描画開始-----

	// ベースUIを描画（ポーズ中以外）
	if (!isPaused_) {
		UI_->Draw();

		// UIPauseを描画（ポーズ中以外で、アルファ値が0より大きい時）
		if (uiPauseAlpha_ > 0.0f) {
			UIPause_->Draw();
		}

		// プレイヤーの攻撃状態に応じて攻撃UIを重ねて描画
		if (player_ && currentPhase_ == GamePhase::Battle) {
			// 右パンチUI
			if (player_->CanRightPunch()) {
				// 実行中なら暗くする
				if (player_->IsRightPunchActive()) {
					attackUI_Right_->SetColor({ 0.4f, 0.4f, 0.4f });
					attackUI_Right_->SetAlpha(1.0f);
				}
				else {
					attackUI_Right_->SetColor({ 1.0f, 1.0f, 1.0f });
					attackUI_Right_->SetAlpha(1.0f);
				}
				attackUI_Right_->Draw();
			}
			// 左パンチUI
			else if (player_->CanLeftPunch()) {
				// 実行中なら暗くする
				if (player_->IsLeftPunchActive()) {
					attackUI_Left_->SetColor({ 0.4f, 0.4f, 0.4f });
					attackUI_Left_->SetAlpha(1.0f);
				}
				else {
					attackUI_Left_->SetColor({ 1.0f, 1.0f, 1.0f });
					attackUI_Left_->SetAlpha(1.0f);
				}
				attackUI_Left_->Draw();
			}
			// ラッシュUI
			else if (player_->CanRush()) {
				// 実行中なら暗くする
				if (player_->IsRushActive()) {
					attackUI_Rush_->SetColor({ 0.4f, 0.4f, 0.4f });
					attackUI_Rush_->SetAlpha(1.0f);
				}
				else {
					attackUI_Rush_->SetColor({ 1.0f, 1.0f, 1.0f });
					attackUI_Rush_->SetAlpha(1.0f);
				}
				attackUI_Rush_->Draw();
			}
		}
	}

	//------------------------

	objCommon_->skinningDrawCommonSetting();
	//-----アニメーションの描画開始-----

	player_->DrawAnimation(vp_);
	enemy_->DrawAnimation(vp_);

	//------------------------------


	objCommon_->DrawCommonSetting();
	//-----3DObjectの描画開始-----

#ifdef _DEBUG
	obj_->Draw(vp_);
#endif // _DEBUG

	player_->Draw(vp_);
	enemy_->Draw(vp_);

	ground_->Draw(vp_);

	//--------------------------

	/// Particleの描画準備
	ptCommon_->DrawCommonSetting();
	//------Particleの描画開始-------
	stageWall_->Draw(Cylinder);
	player_->DrawParticle(vp_);
	enemy_->DrawParticle(vp_);
	//-----------------------------

	//-----線描画-----
	DrawLine3D::GetInstance()->Draw(vp_);
	//---------------

	/// ----------------------------------

	// ポーズ画面の描画（最前面）
	if (isPaused_) {
		spCommon_->DrawCommonSetting();
		DrawPause();
	}

	// ポーズ解除後の再生マークフェードアウト描画
	if (isPlayIconFading_ && playIconAlpha_ > 0.0f) {
		spCommon_->DrawCommonSetting();
		pausePlayIcon_->Draw();
	}

	/// -------描画処理終了-------
}

void GameScene::DrawPause()
{
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

void GameScene::DrawForOffScreen()
{
	/// -------描画処理開始-------

	/// Spriteの描画準備
	spCommon_->DrawCommonSetting();
	//-----Spriteの描画開始-----

	//------------------------

	objCommon_->skinningDrawCommonSetting();
	//-----アニメーションの描画開始-----

	//------------------------------


	objCommon_->DrawCommonSetting();
	//-----3DObjectの描画開始-----

	//--------------------------

	/// Particleの描画準備
	ptCommon_->DrawCommonSetting();
	//------Particleの描画開始-------

	//-----------------------------


	/// ----------------------------------

	/// -------描画処理終了-------
}

void GameScene::Debug()
{
	ImGui::Begin("GameScene:Debug");
	debugCamera_->imgui();
	LightGroup::GetInstance()->imgui();

	// ポーズ状態のデバッグ表示
	ImGui::Checkbox("isPaused", &isPaused_);
	int selection = static_cast<int>(currentSelection_);
	ImGui::SliderInt("Selection", &selection, 0, static_cast<int>(PauseMenuSelection::MaxCount) - 1);

	ImGui::End();

	stageWall_->imgui();
	player_->ImGui();
	enemy_->ImGui();
	ground_->DebugTransform("ground");
}

void GameScene::CameraUpdate()
{
	if (debugCamera_->GetActive()) {
		debugCamera_->Update();
	}
	else {
		followCamera_->Update();
		vp_.matView_ = followCamera_->GetViewProjection().matView_;
		vp_.matProjection_ = followCamera_->GetViewProjection().matProjection_;
		vp_.TransferMatrix();
	}
}

void GameScene::UpdateStart()
{
	// ===== 各オブジェクトの更新 =====
	// --- 敵 ---
	enemy_->UpdateStartEffect();

	// --- プレイヤー ---
	player_->UpdateStartEffect();

	// プレイヤー演出が終了し、まだカメラ移動を開始していなければ開始
	if (player_->GetIsEnd() && !isCameraMoveStart_) {
		followCamera_->SetTarget(&player_->GetWorldTransform());
		followCamera_->StartFollowMove();
		player_->SetFollowCamera(followCamera_.get());
		isCameraMoveStart_ = true; // フラグON
	}

	// カメラ移動が終わったらバトル開始
	if (isCameraMoveStart_ && !followCamera_->IsStartMoving()) {
		currentPhase_ = GamePhase::Battle;
		isCameraMoveStart_ = false; // リセット
	}
}

void GameScene::UpdateBattle()
{
	// ===== 各オブジェクトの更新 =====
	// --- 敵 ---
	enemy_->Update(player_.get(), vp_);

	// --- プレイヤー ---
	player_->Update();

}

void GameScene::ChangeScene()
{
	if (!enemy_->GetIsAlive()) {
		sceneManager_->NextSceneReservation("CLEAR");
	}

	if (player_->GetGameState() == Player::GameState::kGameOver) {
		sceneManager_->NextSceneReservation("OVER");
	}


	//-----------------------------
	if (Input::GetInstance()->TriggerKey(DIK_R)) {

	}
	//-----------------------------

#ifdef _DEBUG
#endif // _DEBUG
}