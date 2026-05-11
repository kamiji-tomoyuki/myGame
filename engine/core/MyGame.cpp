#include "MyGame.h"
#include "SceneFactory.h"
#include <ImGuiManager.h>
#include "BaseScene.h"
#include "ViewProjection.h"

void MyGame::Initialize()
{
	Framework::Initialize();
	Framework::LoadResource();
	Framework::PlaySounds();

	// --- ゲーム固有の処理 ---
	// 最初のシーンの生成
	sceneFactory_ = std::unique_ptr<SceneFactory>(new SceneFactory());

	// シーンマネージャに最初のシーンをセット
	sceneManager_->SetSceneFactory(sceneFactory_.get());
	sceneManager_->NextSceneReservation("TITLE");
}

void MyGame::Finalize()
{
	Framework::Finalize();
}

void MyGame::Update()
{
	Framework::Update();
}

void MyGame::Draw()
{
	dxCommon->PreRenderTexture();
	srvManager->PreDraw();

	object3dCommon->DrawCommonSetting();
	if (sceneManager_->GetTransitionEnd() && sceneManager_->GetBaseScene()) {
		collisionManager_->Draw(*sceneManager_->GetBaseScene()->GetViewProjection());
	}
	sceneManager_->Draw();

	spriteCommon->DrawCommonSetting();
	sceneManager_->DrawTransition();

	dxCommon->PreDraw();
	if (sceneManager_->GetBaseScene()) {
		offscreen_->SetProjection(sceneManager_->GetBaseScene()->GetViewProjection()->matProjection_);
	}
	offscreen_->Draw();
	dxCommon->TransitionDepthBarrier();
	sceneManager_->DrawForOffScreen();

#ifdef _DEBUG
	ImGuiManager::GetInstance()->Draw();
#endif // _DEBUG

	// --- 描画終了 ---
	dxCommon->PostDraw();
}