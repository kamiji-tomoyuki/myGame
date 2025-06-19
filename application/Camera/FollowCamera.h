#pragma once
#include "WorldTransform.h"
#include "ViewProjection.h"

class FollowCamera
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// リセット
	/// </summary>
	void Reset();

public:

	/// 各ステータス取得関数
	/// <returns></returns>
	const ViewProjection& GetViewProjection() { return vp_; }

	/// 各ステータス設定関数
	/// <returns></returns>
	void SetTarget(const WorldTransform* target);

private:

	/// <summary>
	/// ゲームパッド操作
	/// </summary>
	void UpdateGamePad();

	/// <summary>
	/// キーボード操作
	/// </summary>
	void UpdateKeyboard();

	/// <summary>
	/// オフセットの計算
	/// </summary>
	Vector3 MakeOffset();

private:

	// ビュープロジェクション
	ViewProjection vp_;

	// --- 追従対象 ---
	const WorldTransform* target_ = nullptr;
	Vector3 targetPos_{};
	Vector3 offset_ = { 0.0f,2.0f,-10.0f };

	float destinationAngleX_ = -2.9f;
	float destinationAngleY_ = 0.0f;

	Quaternion destinationAngle{};

	Vector3 move{};

};

