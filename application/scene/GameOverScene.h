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
#include <Enemy.h>
#include "Skybox.h"
#include <Stage/Ground.h>
#include "FollowCamera.h"
#include "Sprite.h"

/// <summary>
/// ゲームオーバーシーンクラス
/// </summary>
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
	/// <summary>
	/// デバック
	/// </summary>
	void Debug();

	/// <summary>
	/// カメラの更新
	/// </summary>
	void CameraUpdate();

	/// <summary>
	/// シーン管理
	/// </summary>
	void ChangeScene();

	// タイトル演出の更新
	void UpdateTitleAnimation();

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
	std::unique_ptr<Enemy> enemy_;

	std::unique_ptr<Skybox> skybox_;
	std::unique_ptr<Ground> ground_;

	std::unique_ptr<FollowCamera> followCamera_;

	// --- 各エフェクト・演出 ---
	std::unique_ptr<ParticleEmitter> stageWall_;

	// --- スプライト ---
	std::unique_ptr<Sprite> gameOverTitle_;

	// --- タイトル演出用 ---
	float titleAnimationTimer_ = 0.0f;
	const float kTitleMoveTime = 3.0f;      // 移動時間
	const float kTitleFadeTime = 2.5f;      // フェード時間
	const Vector2 kTitleStartPos = { 240.0f, 0.0f };
	const Vector2 kTitleEndPos = { 240.0f, 180.0f };

	// ふわふわ
	float floatingTimer_ = 0.0f;
	const float kFloatingCycleTime = 2.0f;  // 1サイクルの時間
	const float kFloatingAmplitude = 10.0f; // 揺れ幅(ピクセル)

	bool roop = true;
};