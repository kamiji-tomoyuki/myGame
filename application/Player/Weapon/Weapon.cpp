#include "Weapon.h"

Weapon::Weapon()
{
}

void Weapon::Init()
{
	BaseObject::Init();
}

void Weapon::Update()
{
	BaseObject::Update();
}

void Weapon::UpdateComboTime()
{
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
