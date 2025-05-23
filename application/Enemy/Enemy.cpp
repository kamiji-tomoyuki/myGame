#include "Enemy.h"

#include "CollisionTypeIdDef.h"

#include "myMath.h"

void Enemy::Init()
{
	BaseObject::Init();
	BaseObject::SetWorldPosition(Vector3{ 0.0f,0.0f,15.0f });
	//Collider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayer));

	// --- モデルの初期化 ---
	obj3d_ = std::make_unique<Object3d>();
	obj3d_->Initialize("Enemy/playerArm.gltf");

	// --- 各ステータスの初期値設定 ---
}

void Enemy::Update()
{
	BaseObject::Update();

	// 移動
	Move();

	// アニメーションの再生
	obj3d_->AnimationUpdate(true);
}

void Enemy::Draw(const ViewProjection& viewProjection)
{
}

void Enemy::DrawAnimation(const ViewProjection& viewProjection)
{
	obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection);
}

void Enemy::Move()
{
}
