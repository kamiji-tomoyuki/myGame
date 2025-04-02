#pragma once
#include "ViewProjection.h"

#include "Vector2.h"
#include "Vector3.h"
#include "Matrix4x4.h"

// デバッグカメラ
class DebugCamera
{
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(ViewProjection* viewProjection);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

public:

	/// imGui
	/// <returns></returns>
	void imgui();

	/// 起動
	/// <returns></returns>
	bool GetActive() { return isActive_; }

	// X,Y,Z軸回りのローカル回転角
	Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };
	// ローカル座標
	Vector3 translation_ = { 0.0f, 0.0f, -50.0f };
	Matrix4x4 matRot_;

private:

	void CameraMove(Vector3& cameraRotate, Vector3& cameraTranslate, Vector2& clickPosition);

private:

	ViewProjection* viewProjection_;
	Vector2 mouse;
	bool useMouse = true;
	float mouseSensitivity = 0.005f;
	
	// カメラ移動速度
	float moveZspeed = 0.01f;
	Matrix4x4 matRotDelta;
	Matrix4x4 rotateXYZMatrix;
	bool isActive_ = false;
};

