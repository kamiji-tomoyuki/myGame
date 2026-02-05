#pragma once
#include "BaseObject.h"
#include <Skybox.h>

/// <summary>
/// 地面クラス
/// </summary>
class Ground :public BaseObject
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Init();

	/// <summary>
	/// 更新
	/// </summary>
	void Update()override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw(const ViewProjection& viewProjection) override;

private:

	// --- モデル ---
	std::unique_ptr<Object3d> obj3d_;
	float scale_ = 1000.0f;

};

