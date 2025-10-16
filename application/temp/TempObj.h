#pragma once
#include "BaseObject.h"

/// <summary>
/// デバッグ用オブジェクトクラス
/// </summary>
class TempObj :public BaseObject
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

public:

	/// 各ステータス取得関数
	/// <returns></returns>
	Vector3 GetCenterPosition() const override { return transform_.translation_; }
	Vector3 GetCenterRotation() const override { return transform_.rotation_; }

	/// 各ステータス設定関数
	/// <returns></returns>


};

