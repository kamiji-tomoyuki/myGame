#include "Player.h"
#include "Input.h"

#include "FollowCamera.h"
#include "CollisionTypeIdDef.h"

#include "myMath.h"
#include <Enemy.h>

Player::Player()
{
	serialNumber_ = nextSerialNumber_;
	nextSerialNumber_++;
}

void Player::Init()
{
	BaseObject::Init();
	BaseObject::SetWorldPosition(Vector3{ 0.0f,0.0f,-15.0f });

	Collider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayer));

	// --- モデルの初期化 ---
	obj3d_ = std::make_unique<Object3d>();
	obj3d_->Initialize("walk.gltf");

	// --- 各ステータスの初期値設定 ---
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
	if (isAlive_) {
		obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection);
	}
}

void Player::OnCollision(Collider* other)
{
	uint32_t typeID = other->GetTypeID();

	//衝突相手
	if (typeID == static_cast<uint32_t>(CollisionTypeIdDef::kEnemy)) {
		// 仮処理
		isAlive_ = false;

		Enemy* enemy = static_cast<Enemy*>(other);
		if (enemy->GetSerialNumber() == enemy->GetNextSerialNumber() - 1) {
			return;
		}

		float distance = Vector3(GetCenterPosition() - enemy->GetCenterPosition()).Length();

		Vector3 correction = Vector3(GetCenterPosition() - enemy->GetCenterPosition()).Normalize() * (GetRadius() + enemy->GetRadius() - distance) * 0.750f;
		transform_.translation_ += correction;
		enemy->SetTranslation(enemy->GetTransform().translation_ - correction);

		if (Vector3(GetCenterPosition() - enemy->GetCenterPosition()).Length() < enemy->GetShortDistance()) {
			// プレイヤーとの距離が短い場合は、適切な距離に移動
			Vector3 distance = Vector3(enemy->GetCenterPosition() - GetCenterPosition()).Normalize();
			distance *= enemy->GetShortDistance();
			enemy->SetTranslation(GetCenterPosition() + distance);
		}
	}

	transform_.UpdateMatrix();
}

void Player::Move()
{
	Vector3 move = { 0.0f, 0.0f, 0.0f };

	// --- キーボード ---
	if (Input::GetInstance()->PushKey(DIK_D)) {
		move.x += kAcceleration;
	}
	if (Input::GetInstance()->PushKey(DIK_A)) {
		move.x -= kAcceleration;
	}
	if (Input::GetInstance()->PushKey(DIK_W)) {
		move.z += kAcceleration;
	}
	if (Input::GetInstance()->PushKey(DIK_S)) {
		move.z -= kAcceleration;
	}

	// 回転
	if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
		BaseObject::SetRotationY(BaseObject::GetTransform().rotation_.y + kRotateAcceleration);
	}
	if (Input::GetInstance()->PushKey(DIK_LEFT)) {
		BaseObject::SetRotationY(BaseObject::GetTransform().rotation_.y - kRotateAcceleration);
	}

	// 入力ベクトルがある場合
	if (move.Length() != 0.0f) {
		move.Normalize();  // 正規化して速度が一定になるようにする

		// rotationY の角度に基づく回転行列
		float rotY = BaseObject::GetTransform().rotation_.y;
		Matrix4x4 rotMat = MakeRotateYMatrix(rotY);

		// 向いている方向に移動ベクトルを回転
		move = TransformNormal(move, rotMat);

		velocity_ += move * kAcceleration;  // 加速度をかけて速度に反映
	}
	else {
		velocity_ = 0.0f;
	}

	// 速度上限
	if (velocity_.Length() > kMaxSpeed) {
		velocity_ = velocity_.Normalize() * kMaxSpeed;
	}

	// 位置の更新
	Vector3 pos = BaseObject::GetWorldPosition();
	pos += velocity_;
	BaseObject::SetWorldPosition(pos);


	// --- ゲームパッド ---
}