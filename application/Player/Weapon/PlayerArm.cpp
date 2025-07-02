#include "PlayerArm.h"
#include <CollisionTypeIdDef.h>

#include "Player.h"

PlayerArm::PlayerArm()
{
	serialNumber_ = nextSerialNumber_;
	nextSerialNumber_++;
}

void PlayerArm::Init(std::string filePath)
{
	BaseObject::Init();
	Collider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPArm));

	// --- モデルの初期化 ---
	obj3d_ = std::make_unique<Object3d>();
	obj3d_->Initialize(filePath);
}

void PlayerArm::Update()
{
	BaseObject::Update();

	obj3d_->AnimationUpdate(true);
}

void PlayerArm::UpdateComboTime()
{
	comboTimer_--;

	if (comboTimer_ <= 0) {
		comboTimer_ = 0;
	}
}

void PlayerArm::Draw(const ViewProjection& viewProjection)
{
}

void PlayerArm::DrawAnimation(const ViewProjection& viewProjection)
{
	obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection);
}

void PlayerArm::DrawParticle(const ViewProjection& viewProjection)
{
}

void PlayerArm::ImGui()
{
}

void PlayerArm::OnCollision(Collider* other)
{
}

void PlayerArm::SetPlayer(Player* player)
{
	player_ = player;
	BaseObject::transform_.parent_ = &player->GetWorldTransform();
}
