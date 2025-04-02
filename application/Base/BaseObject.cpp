#include "BaseObject.h"


void BaseObject::Init() {

	/// ワールドトランスフォームの初期化
	transform_.Initialize();
	//カラーのセット
	objColor_.Initialize();
	objColor_.SetColor(Vector4(1, 1, 1, 1));
}

void BaseObject::Update() {

	//元となるワールドトランスフォームの更新
	transform_.UpdateMatrix();
	/// 色転送
	objColor_.TransferMatrix();
}

void BaseObject::Draw(const ViewProjection& viewProjection) {
	obj3d_->Draw(transform_, viewProjection, &objColor_);
}

Vector3 BaseObject::GetWorldPosition() const {
	Vector3 worldPos;
	// ワールド行列の平行移動成分を取得
	worldPos.x = transform_.matWorld_.m[3][0];
	worldPos.y = transform_.matWorld_.m[3][1];
	worldPos.z = transform_.matWorld_.m[3][2];

	return worldPos;
}

void BaseObject::CreateModel(const std::string modelname) {
	obj3d_ = std::make_unique<Object3d>();
	obj3d_->Initialize(modelname);
}

void BaseObject::DebugTransform(const std::string className)
{
	ImGui::Begin(className.c_str());
	ImGui::DragFloat3((className + "位置").c_str(), &transform_.translation_.x, 0.1f);
	float rotationDegrees[3] = {
	radiansToDegrees(transform_.rotation_.x),
	radiansToDegrees(transform_.rotation_.y),
	radiansToDegrees(transform_.rotation_.z)
	};
	if (ImGui::DragFloat3((className + "回転").c_str(), rotationDegrees, 0.1f, -360.0f, 360.0f)) {
		// 操作後、度数法からラジアンに戻して保存
		transform_.rotation_.x = degreesToRadians(rotationDegrees[0]);
		transform_.rotation_.y = degreesToRadians(rotationDegrees[1]);
		transform_.rotation_.z = degreesToRadians(rotationDegrees[2]);
	}
	ImGui::DragFloat3((className + "大きさ").c_str(), &transform_.scale_.x, 0.1f);
	ImGui::End();
}


