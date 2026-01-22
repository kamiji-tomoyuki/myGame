#pragma once
#include "Audio.h"
#include "BaseScene.h"
#include "DebugCamera.h"
#include "Input.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "OffScreen.h"
#include "ParticleCommon.h"
#include "ParticleEmitter.h"
#include "Skybox.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "WorldTransform.h"

#include "application/TitleCharacter.h"

#include "JsonLoader.h"

class TitleScene : public BaseScene {
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

public:

	/// 各ステータス取得関数
	/// <returns></returns>
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

private:
	Audio* audio_;
	Input* input_;
	Object3dCommon* objCommon_;
	SpriteCommon* spCommon_;
	ParticleCommon* ptCommon_;

	ViewProjection vp_;
	std::unique_ptr<DebugCamera> debugCamera_;

	WorldTransform wt1_;
	WorldTransform wt2_;

	std::unique_ptr<Sprite> title2d_;
	std::unique_ptr<Sprite> space_;
        float timer_ = 0.0f;
        float add_ = 1.0f / 60.0f;

	std::unique_ptr<TitleCharacter> player_;

	std::unique_ptr<Skybox> skybox_;

	std::unique_ptr<ParticleEmitter> emitter_;

	std::unique_ptr<OffScreen> offSc_;

	bool roop = true;
};
