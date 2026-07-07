#include "MyGame.h"
#include "SceneFactory.h"
#include <ImGuiManager.h>
#include "BaseScene.h"
#include "ViewProjection.h"
#ifdef _DEBUG
#include "EditorUI.h"
#endif // _DEBUG

namespace Engine {
void MyGame::Initialize()
{
	Framework::Initialize();
	Framework::LoadResource();
	Framework::PlaySounds();

	// --- ゲーム固有の処理 ---
	// 最初のシーンの生成
	sceneFactory_ = std::make_unique<SceneFactory>();

	// シーンマネージャに最初のシーンをセット
	sceneManager_->SetSceneFactory(sceneFactory_.get());
	sceneManager_->NextSceneReservation("TITLE");

#ifdef _DEBUG
	// エディタのシーンメニューに切り替え候補を登録
	EditorUI* editor = EditorUI::GetInstance();
	editor->RegisterScene("TITLE", "タイトル");
	editor->RegisterScene("GAME", "ゲーム");
	editor->RegisterScene("CLEAR", "クリア");
	editor->RegisterScene("OVER", "ゲームオーバー");
	editor->RegisterScene("DEBUG", "デバッグ");
#endif // _DEBUG
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

#ifdef _DEBUG
	// エディタで追加したオブジェクトの描画（レンダーテクスチャへ）
	if (sceneManager_->GetBaseScene()) {
		EditorUI::GetInstance()->DrawEditorObjects(*sceneManager_->GetBaseScene()->GetViewProjection());
	}
#endif // _DEBUG

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
} // namespace Engine
