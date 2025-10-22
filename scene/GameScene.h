#pragma once
#include "Audio.h"
#include "BaseScene.h"
#include "DebugCamera.h"
#include "Input.h"
#include "Object3dCommon.h"
#include "ParticleCommon.h"
#include "SpriteCommon.h"
#include "ViewProjection.h"

#include <Player.h>
#include <Enemy.h>
#include <FollowCamera.h>
#include <Stage/Ground.h>

#ifdef _DEBUG
#include "application/temp/TempObj.h"
#endif //_DEBUG
#include <ParticleEmitter.h>
#include <Skybox.h>

enum class GamePhase {
	EnemyAppear,  // 敵出現演出
	Battle,       // 戦闘パート
};

class GameScene : public BaseScene
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

	void ChangePhase(GamePhase nextPhase);

	void UpdateStart();
	void UpdateBattle();

	void ChangeScene();
private:

	Audio* audio_;
	Input* input_;
	Object3dCommon* objCommon_;
	SpriteCommon* spCommon_;
	ParticleCommon* ptCommon_;

	GamePhase currentPhase_;

	// ビュープロジェクション
	ViewProjection vp_;
	std::unique_ptr<DebugCamera> debugCamera_;

	// --- 各オブジェクト ---
	std::unique_ptr<Player> player_;
	std::unique_ptr<Enemy> enemy_;

	std::unique_ptr<Skybox> skybox_;
	std::unique_ptr<Ground> ground_;

	std::unique_ptr<FollowCamera> followCamera_;

	// --- 各エフェクト・演出 ---
	std::unique_ptr<ParticleEmitter> stageWall_;

	bool isStart = false;
	bool isCameraMoveStart_ = false;

	// --- シーン管理 ---
	bool isClear = false;

	float playT = 0.0f;


#ifdef _DEBUG
	std::unique_ptr<TempObj> obj_;
#endif // _DEBUG
};
