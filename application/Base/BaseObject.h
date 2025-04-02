#pragma once
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "Object3d.h"
#include "ObjColor.h"
#include "engine/utility/collider/Collider.h"
//std
#include<string>

class BaseObject {
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

	// 中心座標取得
	virtual Vector3 GetWorldPosition() const;
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
