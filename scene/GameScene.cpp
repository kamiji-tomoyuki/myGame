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

	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize(&vp_);

	// ===== 各オブジェクトの初期化 =====
	// --- プレイヤー ---
	Player::SetSerialNumber(0);
	player_ = std::make_unique<Player>();
	player_->Init();

	// --- カメラ ---
	followCamera_ = std::make_unique<FollowCamera>();
	followCamera_->Initialize();
	followCamera_->SetTarget(&player_->GetWorldTransform());
	// カメラをプレイヤーに設定
	player_->SetFollowCamera(followCamera_.get());

	// --- 敵 ---
	Enemy::SetSerialNumber(0);
	enemy_ = std::make_unique<Enemy>();
	enemy_->Init();



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

	// ===== 各オブジェクトの更新 =====
	// --- 敵 ---
	enemy_->Update(player_.get());

	// --- プレイヤー ---
	player_->Update();

	// --- カメラ ---
	CameraUpdate();


	// ===== シーン切り替え =====
	ChangeScene();
}

void GameScene::Draw()
{
	/// -------描画処理開始-------

	/// Spriteの描画準備
	spCommon_->DrawCommonSetting();
	//-----Spriteの描画開始-----

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

	//--------------------------

	/// Particleの描画準備
	ptCommon_->DrawCommonSetting();
	//------Particleの描画開始-------

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
	obj_->Update();

	ImGui::Begin("GameScene:Debug");
	debugCamera_->imgui();
	LightGroup::GetInstance()->imgui();
	ImGui::End();
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

void GameScene::ChangeScene()
{
	if (input_->TriggerKey(DIK_SPACE)) {
		sceneManager_->NextSceneReservation("TITLE");
	}
}
