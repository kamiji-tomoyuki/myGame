#include "SceneManager.h"
#include "SceneManagerStates.h"
#include <cassert>
#include <ImGuiManager.h>
#include"GlobalVariables.h"
#include <ModelManager.h>
#include <Player.h>
#include <Enemy.h>

SceneManager* SceneManager::instance = nullptr;

SceneManager* SceneManager::GetInstance()
{
	if (instance == nullptr) {
		instance = new SceneManager;
	}
	return instance;
}

void SceneManager::Initialize()
{
	transition_ = std::make_unique<SceneTransition>();
	transition_->Initialize();

	// 初期状態は通常状態
	state_ = std::make_unique<SceneManagerNormalState>();
}

void SceneManager::Finalize()
{
	if (scene_) {
		firstChange = false;
		scene_->Finalize();
	}
	delete scene_;
	delete instance;
	instance = nullptr;
}

void SceneManager::Update()
{
#ifdef _DEBUG

	ImGui::Begin("scene");
	if (ImGui::Button("TitleScene") && (transition_->IsEnd() && !transition_->FadeInStart())) {
		NextSceneReservation("TITLE");
	}
	if (ImGui::Button("GameScene") && (transition_->IsEnd() && !transition_->FadeInStart())) {
		NextSceneReservation("GAME");
	}
	if (ImGui::Button("GameClearScene") && (transition_->IsEnd() && !transition_->FadeInStart())) {
		NextSceneReservation("CLEAR");
	}
	if (ImGui::Button("GameOverScene") && (transition_->IsEnd() && !transition_->FadeInStart())) {
		NextSceneReservation("OVER");
	}
	if (ImGui::Button("DEBUGScene") && (transition_->IsEnd() && !transition_->FadeInStart())) {
		NextSceneReservation("PARTICLE_DEBUG");
	}
	ImGui::End();

#endif // _DEBUG

	// 状態に応じた更新
	if (state_) {
		state_->Update(this);
	}
}

void SceneManager::Draw()
{
	if (scene_) {
		scene_->Draw();
	}
}

void SceneManager::DrawForOffScreen()
{
	if (scene_) {
		scene_->DrawForOffScreen();
	}
}

void SceneManager::DrawTransition()
{
	if (!transition_->IsEnd()) {
		transition_->Draw(); // トランジションの描画
	}
}

void SceneManager::ChangeState(std::unique_ptr<ISceneManagerState> newState)
{
	state_ = std::move(newState);
}

void SceneManager::NextSceneReservation(const std::string& sceneName)
{
	// トランジション中なら処理をスキップ
	if (!transition_->IsEnd() && transition_->FadeInStart()) {
		return; // すでに遷移中なので、次の遷移予約はしない
	}
	
	// 状態を遷移中に変更
	ChangeState(std::make_unique<SceneManagerTransitionState>());

	transition_->Reset();
	assert(sceneFactory_);
	assert(nextScene_ == nullptr);

	// 次シーンを生成
	nextScene_ = sceneFactory_->CreateScene(sceneName);
	if (!firstChange) {
		transition_->SetFadeOutStart(true);
	}
	else {
		transition_->SetFadeInStart(true);
	}
}

void SceneManager::SceneChange()
{
	if (transition_->FadeInFinish()) {
		// 旧シーンの終了
		if (scene_) {
			scene_->Finalize();
			delete scene_;
			scene_ = nullptr;
		}

		// シーン切り替え時にシリアルナンバーをリセット
		Player::SetSerialNumber(0);
		Enemy::SetSerialNumber(0);
		
		// モデルキャッシュをクリア
		ModelManager::GetInstance()->ClearModels();

		// シーンの切り替え
		scene_ = nextScene_;
		nextScene_ = nullptr;

		// シーンマネージャをセット
		scene_->SetSceneManager(this);

		// 次のシーンを初期化する
		scene_->Initialize();

		transition_->SetFadeOutStart(true);

	}
}

void SceneManager::UpdateTransition()
{
	// 次のシーンの予約があるなら（最初のシーン設定用）
	if (nextScene_) {
		if (!firstChange) {
			transition_->SetFadeInFinish(true);
			firstChange = true;
		}
	}

	if (!transition_->IsEnd()) {
		transition_->Update();
	}
}