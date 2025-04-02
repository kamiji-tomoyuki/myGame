#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"

// カメラ
class Camera
{
public: // メンバ関数
	
	/// <summary>
	/// コンストラクタ
	/// </summary>
	Camera();

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

public:

	/// 各ステータス取得関数
	/// <returns></returns>
	const Matrix4x4& GetWorldMatrix()const { return worldMatrix; }
	const Matrix4x4& GetViewMatrix()const { return viewMatrix; }
	const Matrix4x4& GetProjectionMatrix()const { return projectionMatrix; }
	const Matrix4x4& GetViewProjectionMatrix()const { return viewProjectionMatrix; }
	const Vector3& GetRotate()const { return transform.rotate; }
	const Vector3& GetTranslate()const { return transform.translate; }

	/// 各ステータス設定関数
	/// <returns></returns>
	void SetRotate(const Vector3& rotate) { transform.rotate=rotate; }
	void SetTranslate(const Vector3& translate) { transform.translate = translate; }
	void SetFovY(const float& fovY) { this->fovY = fovY; }
	void SetAspectRatio(const float& aspectRatio) { this->aspectRatio = aspectRatio; }
	void SetNearClip(const float& nearClip) { this->nearClip = nearClip; }
	void SetFarClip(const float& farClip) { this->farClip = farClip; }

private:

	// --- Transform ---
	struct Transform {
		Vector3 scale;
		Vector3 rotate;
		Vector3 translate;
	};

	// --- ビュー行列関連データ ---
	Transform transform;
	Matrix4x4 worldMatrix;
	Matrix4x4 viewMatrix;

	// --- プロジェクション行列関連データ ---
	Matrix4x4 projectionMatrix;
	Matrix4x4 viewProjectionMatrix;
	float fovY;
	float aspectRatio;
	float nearClip;
	float farClip;
};

