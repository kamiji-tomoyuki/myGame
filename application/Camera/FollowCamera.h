#pragma once
#include "WorldTransform.h"
#include "ViewProjection.h"

/// <summary>
/// 追従カメラクラス
/// </summary>
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

	/// <summary>
	/// 目標位置へ移動
	/// </summary>
	void MoveToPosition(const Vector3& targetPosition, float lerpFactor = 0.1f) {
		targetPos_ = Lerp(targetPos_, targetPosition, lerpFactor);
	}

	/// <summary>
	/// 追従開始時のカメラ移動をセットアップ
	/// </summary>
	void StartFollowMove();

public:

	/// @brief 各ステータス取得関数
	const ViewProjection& GetViewProjection() { return vp_; }
	bool IsStartMoving() const { return isStartMove_; }
	bool IsCameraFixed() const { return isCameraFixed_; }

	/// @brief 各ステータス設定関数
	void SetTarget(const WorldTransform* target);
	void SetPosition(const Vector3& position) { targetPos_ = position; }
	void SetRotation(const Vector3& rotation) {
		vp_.rotation_ = rotation;
		destinationAngleX_ = rotation.x;
		destinationAngleY_ = rotation.y;
		destinationAngle = Quaternion::FromEulerAngles(rotation);
	}
	void SetOffset(const Vector3& offset) { offset_ = offset; }
	void SetCameraFixed(bool isFixed);

private:

	/// <summary>
	/// キーボード操作
	/// </summary>
	void UpdateKeyboard();

	/// <summary>
	/// ゲームパッド操作
	/// </summary>
	void UpdateGamePad();

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

private:

	bool isStartMove_ = false;
	float startMoveTime_ = 0.0f;
	float startMoveDuration_ = 120.0f; // 約2秒
	Vector3 startPos_;
	Vector3 targetStartPos_;

	// カメラ固定機能
	bool isCameraFixed_ = false;
	Vector3 fixedPosition_{};
	Vector3 fixedRotation_{};
	Quaternion fixedQuaternion_{};

};