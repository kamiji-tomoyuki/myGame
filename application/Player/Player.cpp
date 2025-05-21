#include "Player.h"
#include "Input.h"

void Player::Init()
{
	BaseObject::Init();

	// --- モデルの初期化 ---
	obj3d_ = std::make_unique<Object3d>();
	obj3d_->Initialize("walk.gltf");

	// --- 各ステータスの初期値設定 ---
	kAcceleration = 0.1f;
}

void Player::Update()
{
	BaseObject::Update();

	// 移動
	Move();

	// アニメーションの再生
	obj3d_->AnimationUpdate(true);
}

void Player::Draw(const ViewProjection& viewProjection)
{
	
}

void Player::DrawAnimation(const ViewProjection& viewProjection)
{
	obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection);
}

void Player::Move()
{
	if (Input::GetInstance()->PushKey(DIK_D)) {
		BaseObject::SetWorldPositionX(BaseObject::GetWorldPosition().x + kAcceleration);
	}
	if (Input::GetInstance()->PushKey(DIK_A)) {
		BaseObject::SetWorldPositionX(BaseObject::GetWorldPosition().x - kAcceleration);
	}
	if (Input::GetInstance()->PushKey(DIK_W)) {
		BaseObject::SetWorldPositionZ(BaseObject::GetWorldPosition().z + kAcceleration);
	}
	if (Input::GetInstance()->PushKey(DIK_S)) {
		BaseObject::SetWorldPositionZ(BaseObject::GetWorldPosition().z - kAcceleration);
	}
}
