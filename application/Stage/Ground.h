#pragma once
#include "BaseObject.h"

class Ground :public BaseObject
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Init()override;

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

};

