#include "GameOverScene.h"
#include "ImGuiManager.h"
#include "SceneManager.h"
#include "SrvManager.h"

#ifdef _DEBUG
#include <imgui.h>
#endif // _DEBUG

#include <LightGroup.h>
#include <line/DrawLine3D.h>

void GameOverScene::Initialize()
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

	wt1_.Initialize();

	wt1_.translation_ = { -2.0f,0.0f,0.0f };

	walk_ = std::make_unique<Object3d>();
	walk_->Initialize("Player/player.gltf");

	obb = std::make_unique<Object3d>();
	obb->Initialize("walk.gltf");

	emitter_ = std::make_unique<ParticleEmitter>();
	emitter_->Initialize("test", "debug/plane.obj");

	json_ = std::make_unique<JsonLoader>();
	std::string filePath = "scene/test.json";
	std::string targetName = "ICO球";

	Vector3 position = json_->GetWorldTransform(filePath, targetName);
	wt1_.translation_ = position;
}

void GameOverScene::Finalize()
{

}

void GameOverScene::Update()
{
#ifdef _DEBUG
	// デバッグ
	Debug();
#endif // _DEBUG

	// カメラ更新
	CameraUpdate();

	// シーン切り替え
	ChangeScene();

	emitter_->Update(vp_);
	walk_->UpdateAnimation(roop);

	wt1_.UpdateMatrix();
}

void GameOverScene::Draw()
{
	/// -------描画処理開始-------

	emitter_->DrawEmitter();

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
	//emitter_->Draw(Normal);
	//-----------------------------

	//-----線描画-----
	DrawLine3D::GetInstance()->Draw(vp_);
	//---------------

	/// ----------------------------------

	/// -------描画処理終了-------
}

void GameOverScene::DrawForOffScreen()
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


void GameOverScene::Debug()
{
	ImGui::Begin("GameOverScene:Debug");

	debugCamera_->imgui();

	LightGroup::GetInstance()->imgui();

	ImGui::Checkbox("roop", &roop);

	ImGui::End();

	emitter_->imgui();
}

void GameOverScene::CameraUpdate()
{
	if (debugCamera_->GetActive()) {
		debugCamera_->Update();
	}
	else {
		vp_.UpdateMatrix();
	}
}

void GameOverScene::ChangeScene()
{
	if (input_->TriggerKey(DIK_SPACE)) {
		sceneManager_->NextSceneReservation("TITLE");
	}
}
