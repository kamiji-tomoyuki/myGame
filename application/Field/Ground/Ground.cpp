#include "Ground.h"

void Ground::Init()
{
	BaseObject::Init();
	BaseObject::SetScale({ 1000.0f,1000.0f, 1000.0f });
	BaseObject::SetWorldPositionY(-1.5f);

	// --- モデルの初期化 ---
	obj3d_ = std::make_unique<Object3d>();
	obj3d_->Initialize("debug/Ground.obj");
}

void Ground::Update()
{
	BaseObject::Update();
}

void Ground::Draw(const ViewProjection& viewProjection)
{
	obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection);
}
