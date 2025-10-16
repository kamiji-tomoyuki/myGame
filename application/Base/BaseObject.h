#pragma once
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "Object3d.h"
#include "ObjColor.h"
#include "Collider.h"
//std
#include<string>

/// <summary>
/// オブジェクト基底クラス
/// </summary>
class BaseObject : public Collider {
protected:

	/// ===================================================
	///protected variaus
	/// ===================================================

	// モデル配列データ
	std::unique_ptr<Object3d> obj3d_;
	// ベースのワールド変換データ
	WorldTransform transform_;
	//カラー
	ObjColor objColor_;

public:

	/// ===================================================
	///public method
	/// ===================================================

	//初期化、更新、描画
	virtual void Init();
	virtual void Update();
	virtual void Draw(const ViewProjection& viewProjection);

	virtual void CreateModel(const std::string modelname);

	virtual void DebugTransform(const std::string className);

	/// <summary>
	/// 当たってる間
	/// </summary>
	/// <param name="other"></param>
	virtual void OnCollision([[maybe_unused]] Collider* other) override {};

	/// <summary>
	/// 当たった瞬間
	/// </summary>
	/// <param name="other"></param>
	virtual void OnCollisionEnter([[maybe_unused]] Collider* other) override {};

	/// <summary>
	/// 当たり終わった瞬間
	/// </summary>
	/// <param name="other"></param>
	virtual void OnCollisionOut([[maybe_unused]] Collider* other) override {};

	// 中心座標を取得
	virtual Vector3 GetCenterPosition() const override;
	virtual Vector3 GetCenterRotation() const override;

	// 中心座標取得
	virtual Vector3 GetWorldPosition() const;
	virtual Vector3 GetWorldRotation() const;
	virtual const WorldTransform& GetWorldTransform() const { return transform_; }

	/// ===================================================
	///getter 
	/// ===================================================
	const WorldTransform& GetTransform() { return transform_; }

	/// ===================================================
	///setter 
	/// ===================================================
	void SetObjColor(Vector4 c) { objColor_.SetColor(c); }
	void SetWorldPosition(Vector3 pos) { transform_.translation_ = pos; }
	void SetWorldPositionY(float pos) { transform_.translation_.y = pos; }
	void SetWorldPositionX(float pos) { transform_.translation_.x = pos; }
	void SetWorldPositionZ(float pos) { transform_.translation_.z = pos; }
	void AddPosition(Vector3 pos) { transform_.translation_ += pos; }
	void SetRotation(Vector3 rotate) { transform_.rotation_ = rotate; }
	void SetRotationY(float rotate) { transform_.rotation_.y = rotate; }
	void SetScale(Vector3 scale) { transform_.scale_ = scale; }
};
