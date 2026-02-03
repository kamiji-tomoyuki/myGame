#include "GameClearScene.h"
#include "ImGuiManager.h"
#include "SceneManager.h"
#include "SrvManager.h"
#include "Easing.h"

#ifdef _DEBUG
#include <imgui.h>
#endif // _DEBUG

#include <LightGroup.h>
#include <line/DrawLine3D.h>

void GameClearScene::Initialize()
{
	audio_ = Audio::GetInstance();
	objCommon_ = Object3dCommon::GetInstance();
	spCommon_ = SpriteCommon::GetInstance();
	ptCommon_ = ParticleCommon::GetInstance();
	input_ = Input::GetInstance();

	vp_.Initialize();
	vp_.translation_ = { 0.0f,0.0f,-10.0f };

	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize(&vp_);

	player_ = std::make_unique<Player>();
	player_->Init();
	player_->SetViewProjection(&vp_);
	player_->SetScale({ 1.0f,1.0f,1.0f });
	// ゲームクリア状態に設定
	player_->SetGameState(Player::GameState::kGameClear);

	enemy_ = std::make_unique<Enemy>();
	enemy_->Init();
	// ゲームクリア状態に設定
	enemy_->SetGameState(Enemy::GameState::kGameClear);

	followCamera_ = std::make_unique<FollowCamera>();
	followCamera_->Initialize();
	followCamera_->SetTarget(&player_->GetWorldTransform());
	followCamera_->SetCameraFixed(true);

	// --- ステージ ---
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize("skybox.dds");

	ground_ = std::make_unique<Ground>();
	ground_->Init();

	// ===== 各エフェクト・演出の初期化 =====
	stageWall_ = std::make_unique<ParticleEmitter>();
	stageWall_->Initialize("stage", "debug/ringPlane.obj");

	// ===== スプライト =====
	gameClearTitle_ = std::make_unique<Sprite>();
	gameClearTitle_->Initialize("gameclear.png", kTitleStartPos);

	// タイマー初期化
	titleAnimationTimer_ = 0.0f;
}

void GameClearScene::Finalize()
{

}

void GameClearScene::Update()
{
#ifdef _DEBUG
	// デバッグ
	Debug();
#endif // _DEBUG

	// カメラ更新
	CameraUpdate();

	// タイトル演出の更新
	UpdateTitleAnimation();

	// シーン切り替え
	ChangeScene();

	// --- フィールド ---
	skybox_->Update(vp_);
	ground_->Update();

	// ===== 各エフェクト・演出の更新 =====
	stageWall_->Update(vp_);

	player_->Update();
	enemy_->Update(player_.get(), vp_);
}

void GameClearScene::Draw()
{
	/// -------描画処理開始-------

	skybox_->Draw();

	/// Spriteの描画準備
	spCommon_->DrawCommonSetting();
	//-----Spriteの描画開始-----
	gameClearTitle_->Draw();
	//------------------------

	objCommon_->skinningDrawCommonSetting();
	//-----アニメーションの描画開始-----

	player_->DrawAnimation(vp_);
	enemy_->DrawAnimation(vp_);

	//------------------------------

	objCommon_->DrawCommonSetting();
	//-----3DObjectの描画開始-----

	player_->Draw(vp_);
	enemy_->Draw(vp_);

	ground_->Draw(vp_);

	//--------------------------

	/// Particleの描画準備
	ptCommon_->DrawCommonSetting();
	//------Particleの描画開始-------

	stageWall_->Draw(Cylinder);

	//-----------------------------

	//-----線描画-----
	DrawLine3D::GetInstance()->Draw(vp_);
	//---------------

	/// ----------------------------------

	/// -------描画処理終了-------
}

void GameClearScene::DrawForOffScreen()
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


void GameClearScene::Debug()
{
	ImGui::Begin("GameClearScene:Debug");

	debugCamera_->imgui();

	LightGroup::GetInstance()->imgui();

	ImGui::Checkbox("loop", &loop);

	// タイトル演出のデバッグ表示
	ImGui::Text("Title Animation Timer: %.2f", titleAnimationTimer_);
	ImGui::Text("Floating Timer: %.2f", floatingTimer_);
	Vector2 currentPos = gameClearTitle_->GetPosition();
	ImGui::Text("Title Position: (%.1f, %.1f)", currentPos.x, currentPos.y);
	ImGui::Text("Title Alpha: %.2f", gameClearTitle_->GetColor().w);

	ImGui::End();
}

void GameClearScene::CameraUpdate()
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

void GameClearScene::ChangeScene()
{
	if (input_->TriggerKey(DIK_SPACE)) {
		sceneManager_->NextSceneReservation("TITLE");
	}
}

void GameClearScene::UpdateTitleAnimation()
{
	// タイマー更新
	titleAnimationTimer_ += 1.0f / 60.0f; // 60FPS想定

	// 位置の更新(3秒かけて移動、EaseOutQuadで減速)
	if (titleAnimationTimer_ <= kTitleMoveTime) {
		Vector2 newPos = EaseOutQuad(kTitleStartPos, kTitleEndPos, titleAnimationTimer_, kTitleMoveTime);
		gameClearTitle_->SetPosition(newPos);
	}
	else {
		// 移動完了後はふわふわアニメーション
		floatingTimer_ += 1.0f / 60.0f;

		// サイクルタイムで正規化(0.0~1.0のループ)
		float normalizedTime = std::fmodf(floatingTimer_, kFloatingCycleTime) / kFloatingCycleTime;

		// EaseInOutSineで滑らかな上下動(0→1→0の動き)
		float offset = EaseInOutSine(-kFloatingAmplitude, kFloatingAmplitude, normalizedTime, 1.0f);

		Vector2 floatingPos = kTitleEndPos;
		floatingPos.y += offset;
		gameClearTitle_->SetPosition(floatingPos);
	}

	// 透明度の更新(2.5秒かけてフェードイン)
	if (titleAnimationTimer_ <= kTitleFadeTime) {
		float alpha = EaseOutQuad(0.0f, 1.0f, titleAnimationTimer_, kTitleFadeTime);
		gameClearTitle_->SetAlpha(alpha);
	}
	else {
		// フェード完了後は完全不透明に固定
		gameClearTitle_->SetAlpha(1.0f);
	}
}