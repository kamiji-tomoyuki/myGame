#pragma once
#include "GlobalVariables.h"
#include "Object3d.h"
#include "ViewProjection.h"
#include "WorldTransform.h"

#include "Vector3.h"

struct AABB {
	Vector3 min; //!< 最小点
	Vector3 max; //!< 最大点
};

struct OBB {
	Vector3 center;          //!< 中心点
	Vector3 orientations[3]; //!< 座標軸。正規化・直行必須
	Vector3 size;            //!< 座標軸方向の長さの半分。中心から面までの距離
};

// 当たり判定管理
class Collider {
public:

	Collider();

	/// <summary>
	/// デストラクタ
	/// </summary>
	virtual ~Collider();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// ワールドトランスフォームの更新
	/// </summary>
	void UpdateWorldTransform();

	/// <summary>
	/// 描画
	/// </summary>
	/// <param name="model"></param>
	/// <param name="viewProjection"></param>
	void DrawSphere(const ViewProjection& viewProjection);

	/// <summary>
	/// 描画
	/// </summary>
	/// <param name="model"></param>
	/// <param name="viewProjection"></param>
	void DrawAABB(const ViewProjection& viewProjection);

	void DrawOBB(const ViewProjection& viewProjection);

	/// <summary>
	/// 当たってる間
	/// </summary>
	/// <param name="other"></param>
	virtual void OnCollision([[maybe_unused]] Collider* other) {};

	/// <summary>
	/// 当たった瞬間
	/// </summary>
	/// <param name="other"></param>
	virtual void OnCollisionEnter([[maybe_unused]] Collider* other) {};

	/// <summary>
	/// 当たり終わった瞬間
	/// </summary>
	/// <param name="other"></param>
	virtual void OnCollisionOut([[maybe_unused]] Collider* other) {};

	/// 各ステータス取得関数
	/// <returns></returns>
	float GetRadius() { return radius_; }
	virtual Vector3 GetCenterPosition() const = 0;
	virtual Vector3 GetCenterRotation() const = 0;
	Vector3 GetCenter() { return Cubewt_.translation_; }
	AABB GetAABB() { return aabb; }
	OBB GetOBB() { return obb; }
	bool IsCollisionEnabled() const { return isCollisionEnabled_; }
	bool IsColliding() const { return isColliding; }
	bool WasColliding() const { return wasColliding; }

	/// 各ステータス設定関数
	/// <returns></returns>
	void SetRadius(float radius) { radius_ = radius; }
	void SetIsColliding(bool colliding) { wasColliding = isColliding; isColliding = colliding; }
	void SetCollisionEnabled(bool enabled) { isCollisionEnabled_ = enabled; }
	void SetAABBScale(Vector3 scale) { scale_ = scale; }
	void SetHitColor() { color_ = { 1.0f,0.0f,0.0f,1.0f }; }
	void SetDefaultColor() { color_ = { 1.0f,1.0f,1.0f,1.0f }; }
	
private:
	void ApplyVariables();
	void MakeOBBOrientations(OBB& obb, const Vector3& rotate);

private:

	// 衝突半径
	float radius_ = 1.0f;
	// ワールドトランスフォーム
	WorldTransform Cubewt_;
	WorldTransform AABBwt_;
	WorldTransform OBBwt_;
	// 種別ID
	std::unique_ptr<Object3d>sphere_;
	std::unique_ptr<Object3d>AABB_;
	std::unique_ptr<Object3d>OBB_;

	GlobalVariables* variables_;
	std::string groupName;
	AABB aabb;
	OBB obb;
	Vector3 aabbCenter;
	Vector3 aabbScale;
	Vector3 scale_ = { 1.0f,1.0f,1.0f };
	Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };

	static int counter; // 静的カウンタ
	Vector3 SphereOffset = { 0.0f,0.0f,0.0f };
	AABB AABBOffset;
	OBB OBBOffset;

	bool isCollisionEnabled_ = true;  // デフォルトではコリジョンを有効化
	bool isColliding = false;   // 現在のフレームの衝突状態
	bool wasColliding = false;  // 前フレームの衝突状態
};
