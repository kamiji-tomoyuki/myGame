#include "GameScene.h"
#include "SceneManager.h"

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
#ifdef _DEBUG
	// デバッグ
	Debug();
#endif // _DEBUG

	// --- カメラ ---
	CameraUpdate();

	// ===== 各オブジェクトの更新 =====
	// --- フィールド ---
	skybox_->Update(vp_);
	ground_->Update();

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

void GameScene::Draw()
{
	/// -------描画処理開始-------

	skybox_->Draw();

	/// Spriteの描画準備
	spCommon_->DrawCommonSetting();
	//-----Spriteの描画開始-----

	// ベースUIを描画
	UI_->Draw();

	// プレイヤーの攻撃状態に応じて攻撃UIを重ねて描画
	if (player_ && currentPhase_ == GamePhase::Battle) {
		// 右パンチUI
		if (player_->CanRightPunch()) {
			// 実行中なら暗くする
			if (player_->IsRightPunchActive()) {
				attackUI_Right_->SetColor(Vector3(0.4f, 0.4f, 0.4f));
			}
			else {
				attackUI_Right_->SetColor(Vector3(1.0f, 1.0f, 1.0f));
			}
			attackUI_Right_->Draw();
		}
		// 左パンチUI
		else if (player_->CanLeftPunch()) {
			// 実行中なら暗くする
			if (player_->IsLeftPunchActive()) {
				attackUI_Left_->SetColor(Vector3(0.4f, 0.4f, 0.4f));
			}
			else {
				attackUI_Left_->SetColor(Vector3(1.0f, 1.0f, 1.0f));
			}
			attackUI_Left_->Draw();
		}
		// ラッシュUI
		else if (player_->CanRush()) {
			// 実行中なら暗くする
			if (player_->IsRushActive()) {
				attackUI_Rush_->SetColor(Vector3(0.4f, 0.4f, 0.4f));
			}
			else {
				attackUI_Rush_->SetColor(Vector3(1.0f, 1.0f, 1.0f));
			}
			attackUI_Rush_->Draw();
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
	ImGui::Begin("GameScene:Debug");
	debugCamera_->imgui();
	LightGroup::GetInstance()->imgui();
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

#ifdef _DEBUG
#endif // _DEBUG
}