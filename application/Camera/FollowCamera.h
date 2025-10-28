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

	/// <summary>
	/// カメラ位置を設定（追従対象がない場合の基準位置）
	/// </summary>
	void SetPosition(const Vector3& position) { targetPos_ = position; }

	/// <summary>
	/// カメラの回転を設定
	/// </summary>
	void SetRotation(const Vector3& rotation) {
		vp_.rotation_ = rotation;
		destinationAngleX_ = rotation.x;
		destinationAngleY_ = rotation.y;
		destinationAngle = Quaternion::FromEulerAngles(rotation);
	}

	/// <summary>
	/// カメラのオフセットを設定
	/// </summary>
	void SetOffset(const Vector3& offset) { offset_ = offset; }

	/// <summary>
	/// 目標位置へ移動（補間付き）
	/// </summary>
	void MoveToPosition(const Vector3& targetPosition, float lerpFactor = 0.1f) {
		targetPos_ = Lerp(targetPos_, targetPosition, lerpFactor);
	}

	// 追従開始時のカメラ移動をセットアップ
	void StartFollowMove();

	// 追従開始演出中か？
	bool IsStartMoving() const { return isStartMove_; }

	/// <summary>
	/// カメラ固定設定
	/// </summary>
	/// <param name="isFixed">true: 固定, false: 追従</param>
	void SetCameraFixed(bool isFixed);

	/// <summary>
	/// カメラが固定されているか
	/// </summary>
	bool IsCameraFixed() const { return isCameraFixed_; }

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