#include "GameScene.h"
#include "SceneManager.h"
#include "Easing.h"
#include <cmath>

#include <LightGroup.h>
#include <line/DrawLine3D.h>
#ifdef _DEBUG
#include "EditorUI.h"
#endif // _DEBUG

using namespace Engine;
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
	enemy_->SetViewProjection(&vp_);
	enemy_->SetGameState(Enemy::GameState::kPlaying);

	player_->SetEnemy(enemy_.get());

	// --- ステージ ---
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize("skybox.dds");

	ground_ = std::make_unique<Ground>();
	ground_->SetSkybox(skybox_.get());
	ground_->Init();

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

	// ポーズ表示用UI初期化
	UIPause_ = std::make_unique<Sprite>();
	UIPause_->Initialize("gameUIPause.png", { 40.0f, 60.0f });
	UIPause_->SetAlpha(1.0f); // 初期状態から表示

	// ===== ポーズ管理クラスの初期化 =====
	pause_ = std::make_unique<Pause>();
	pause_->Initialize();

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
		pause_->TogglePause();
	}
	prevPauseKeyState_ = currentPauseKeyState;

	// --- カメラ更新（ポーズ中でも実行） ---
	CameraUpdate();

	// --- フィールド更新（ポーズ中でも実行して画面に表示） ---
	skybox_->Update(vp_);
	ground_->Update();

	// ポーズ中の処理
	if (pause_->IsPaused()) {
		pause_->Update();

		// ポーズメニューからのシーン切り替え要求をチェック
		if (pause_->IsRetryRequested()) {
			sceneManager_->NextSceneReservation("GAME");
			pause_->ResetRequests();
		}
		if (pause_->IsBackToTitleRequested()) {
			sceneManager_->NextSceneReservation("TITLE");
			pause_->ResetRequests();
		}

		return; // ポーズ中はゲームロジックの更新を行わない
	}

	// 再生マークのフェードアウト処理（ポーズ解除後）
	pause_->UpdatePlayIconFade();

	// UIPauseのフェード更新（再生マークが消えた後にフェードインするロジックはPauseクラス内）
	pause_->UpdateUIPauseFade();
	UIPause_->SetAlpha(pause_->GetUIPauseAlpha());

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
}

void GameScene::Draw()
{
	/// -------描画処理開始-------

	// 1. スカイボックス (3D背景)
	skybox_->Draw();

	// 2. 3Dモデル (アニメーション)
	objCommon_->skinningDrawCommonSetting();
	player_->DrawAnimation(vp_);
	enemy_->DrawAnimation(vp_);

	// 3. 3Dモデル (通常)
	objCommon_->DrawCommonSetting();
#ifdef _DEBUG
	if (obj_) { obj_->Draw(vp_); }
#endif // _DEBUG
	player_->Draw(vp_);
	enemy_->Draw(vp_);
	ground_->Draw(vp_);

	// 4. パーティクル
	ptCommon_->DrawCommonSetting();
	stageWall_->Draw(Cylinder);
	player_->DrawParticle(vp_);
	enemy_->DrawParticle(vp_);

	// 5. ライン (デバッグ用など)
	DrawLine3D::GetInstance()->Draw(vp_);

	// 6. スプライト (2D/UI)
	spCommon_->DrawCommonSetting();

	// ゲーム中のUI (ポーズ中でない、かつフェードアウト演出中でもない場合のみ)
	if (!pause_->IsPaused() && !pause_->IsPlayIconFading()) {
		UI_->Draw();

		if (currentPhase_ == GamePhase::Battle) {
			if (player_) {
				// 右パンチUI
				if (player_->CanRightPunch()) {
					if (player_->IsRightPunchActive()) {
						attackUI_Right_->SetColor({ 0.4f, 0.4f, 0.4f });
					}
					else {
						attackUI_Right_->SetColor({ 1.0f, 1.0f, 1.0f });
					}
					attackUI_Right_->SetAlpha(1.0f);
					attackUI_Right_->Draw();
				}
				// 左パンチUI
				else if (player_->CanLeftPunch()) {
					if (player_->IsLeftPunchActive()) {
						attackUI_Left_->SetColor({ 0.4f, 0.4f, 0.4f });
					}
					else {
						attackUI_Left_->SetColor({ 1.0f, 1.0f, 1.0f });
					}
					attackUI_Left_->SetAlpha(1.0f);
					attackUI_Left_->Draw();
				}
				// ラッシュUI
				else if (player_->CanRush()) {
					if (player_->IsRushActive()) {
						attackUI_Rush_->SetColor({ 0.4f, 0.4f, 0.4f });
					}
					else {
						attackUI_Rush_->SetColor({ 1.0f, 1.0f, 1.0f });
					}
					attackUI_Rush_->SetAlpha(1.0f);
					attackUI_Rush_->Draw();
				}
			}
		}

		// HPバー
		player_->DrawSprite(vp_);
		enemy_->DrawSprite(vp_);
	}

	// UIPause (ポーズ中でない場合のみ)
	if (!pause_->IsPaused()) {
		UIPause_->Draw();
	}

	// ポーズ画面 (最前面)
	if (pause_->IsPaused()) {
		pause_->Draw();
	}

	// ポーズ解除後の再生マークフェードアウト描画
	if (pause_->IsPlayIconFading()) {
		pause_->DrawPlayIcon();
	}

	/// -------描画処理終了-------
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
#ifdef _DEBUG
	// 表示メニュー（EditorUI）から各パネルの表示/非表示を切り替え可能にする
	EditorUI* editor = EditorUI::GetInstance();

	if (editor->PanelVisible("カメラ / ライト", "シーン")) {
		ImGui::Begin("GameScene:Debug");
		debugCamera_->imgui();
		LightGroup::GetInstance()->imgui();
		ImGui::End();
	}
	if (editor->PanelVisible("ステージ壁", "パーティクル")) { stageWall_->imgui(); }
	if (editor->PanelVisible("プレイヤー", "キャラクター")) { player_->ImGui(); }
	if (editor->PanelVisible("敵", "キャラクター")) { enemy_->ImGui(); }
	if (editor->PanelVisible("地面", "シーン")) { ground_->DebugTransform("ground"); }
#endif // _DEBUG
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