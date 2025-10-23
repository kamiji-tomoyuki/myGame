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

#include "JsonLoader.h"

/// <summary>
/// タイトルシーンクラス
/// </summary>
class TitleScene :public BaseScene
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

	std::unique_ptr<Object3d> walk_;

	std::unique_ptr<ParticleEmitter> emitter_;

	std::unique_ptr<JsonLoader> json_;

	bool roop = true;
};
