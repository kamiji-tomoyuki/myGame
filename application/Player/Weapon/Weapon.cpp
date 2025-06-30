#include "Weapon.h"
#include <CollisionTypeIdDef.h>

Weapon::Weapon()
{
}

void Weapon::Init()
{
	BaseObject::Init();
	Collider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPWeapon));

}

void Weapon::Update()
{
	BaseObject::Update();
}

void Weapon::UpdateComboTime()
{
	comboTimer_--;

	if (comboTimer_ <= 0) {
		comboTimer_ = 0;
	}
}

void Weapon::Draw(const ViewProjection& viewProjection)
{
}

void Weapon::DrawAnimation(const ViewProjection& viewProjection)
{
}

void Weapon::DrawParticle(const ViewProjection& viewProjection)
{
}

void Weapon::ImGui()
{
}

void Weapon::OnCollision(Collider* other)
{
}

void Weapon::SetPlayer(Player* player)
{
	player_ = player;
	transform_.parent_ = &player->GetWorldTransform();
}
