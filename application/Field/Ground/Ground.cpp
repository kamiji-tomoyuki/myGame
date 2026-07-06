#include "Ground.h"

using namespace Engine;
void Ground::Init()
{
	BaseObject::Init();
	BaseObject::SetScale({ scale_, scale_, scale_ });
	BaseObject::SetWorldPositionY(kGroundOffsetY_);

	// --- モデルの初期化 ---
	obj3d_ = std::make_unique<Object3d>();
	obj3d_->Initialize("debug/Ground.obj");

	if (skybox_) {
		obj3d_->GetModel()->SetEnvironmentSrvIndex(skybox_->GetTextureIndex());
		obj3d_->SetRefrect(0.6f);
	}
}

void Ground::Update()
{
	BaseObject::Update();
}

void Ground::Draw(const ViewProjection& viewProjection)
{
	//obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection);
}