#pragma once
#include "Audio.h"
#include "BaseScene.h"
#include "DebugCamera.h"
#include "Input.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "ParticleCommon.h"
#include "ParticleEmitter.h"
#include "SpriteCommon.h"
#include "WorldTransform.h"

#include "Player.h"
#include "Skybox.h"
#include <Stage/Ground.h>
#include "FollowCamera.h"
#include "Sprite.h"

class GameOverScene :public BaseScene
{
public: // メンバ関数

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize()override;

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize()override;

	/// <summary>
	/// 更新
	/// </summary>
	void Update()override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw()override;

	/// <summary>
	/// オフスクリーン上に描画
	/// </summary>
	void DrawForOffScreen()override;

	ViewProjection* GetViewProjection()override { return &vp_; }

private:
	void Debug();

	void CameraUpdate();

	void ChangeScene();

private:
	Audio* audio_;
	Input* input_;
	Object3dCommon* objCommon_;
	SpriteCommon* spCommon_;
	ParticleCommon* ptCommon_;

	ViewProjection vp_;
	std::unique_ptr<DebugCamera> debugCamera_;

	WorldTransform wt1_;

	std::unique_ptr<Player> player_;

	std::unique_ptr<Skybox> skybox_;
	std::unique_ptr<Ground> ground_;

	std::unique_ptr<FollowCamera> followCamera_;

	// --- 各エフェクト・演出 ---
	std::unique_ptr<ParticleEmitter> stageWall_;

	// --- スプライト ---
	std::unique_ptr<Sprite> gameOverTitle_;

	bool roop = true;
};
