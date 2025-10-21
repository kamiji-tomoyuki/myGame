#include "Ground.h"

void Ground::Init(Skybox* skybox)
{
	BaseObject::Init();
	BaseObject::SetScale({ 1000.0f,1000.0f, 1000.0f });
	BaseObject::SetWorldPositionY(-2.5f);

	// --- モデルの初期化 ---
	obj3d_ = std::make_unique<Object3d>();
	obj3d_->Initialize("debug/Ground.obj");

	obj3d_->GetModel()->SetEnvironmentSrvIndex(skybox->GetTextureIndex());
	obj3d_->SetRefrect(true);
}

void Ground::Update()
{
	BaseObject::Update();
}

void Ground::Draw(const ViewProjection& viewProjection)
{
	obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection);
}
