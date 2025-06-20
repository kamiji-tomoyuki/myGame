#include "Enemy.h"

#include "CollisionTypeIdDef.h"
#include "Player.h"

#include "myMath.h"

uint32_t Enemy::nextSerialNumber_ = 0;

Enemy::Enemy()
{
	// シリアルナンバーをふる
	serialNumber_ = nextSerialNumber_;
	// 次の番号を加算
	++nextSerialNumber_;
}

void Enemy::Init()
{
	BaseObject::Init();
	BaseObject::SetWorldPosition(Vector3{ 0.0f,0.0f,15.0f });

	Collider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kEnemy));

	// --- モデルの初期化 ---
	obj3d_ = std::make_unique<Object3d>();
	obj3d_->Initialize("Enemy/playerArm.gltf");

	// --- 各ステータスの初期値設定 ---

}

void Enemy::Update(Player* player)
{
	player_ = player;
	BaseObject::Update();

	// 接近
	Approach();

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

void Enemy::OnCollision(Collider* other)
{
	uint32_t typeID = other->GetTypeID();

	// プレイヤーとの衝突処理
	if (typeID == static_cast<uint32_t>(CollisionTypeIdDef::kPlayer)) {
		Player* player = static_cast<Player*>(other);

		// 重複処理を避けるため、シリアル番号の小さい方で処理
		if (GetSerialNumber() > player->GetSerialNumber()) {
			return;
		}

		// 衝突時の反発処理
		HandleCollisionWithPlayer(player);
	}
}

void Enemy::HandleCollisionWithPlayer(Player* player)
{
	Vector3 enemyPos = GetCenterPosition();
	Vector3 playerPos = player->GetCenterPosition();

	// 距離計算
	Vector3 direction = enemyPos - playerPos;
	float distance = direction.Length();

	// 重なりがある場合のみ処理
	float totalRadius = GetRadius() + player->GetRadius();
	if (distance < totalRadius && distance > 0.0f) {
		// 正規化された方向ベクトル
		direction = direction.Normalize();

		// 重なり量
		float overlap = totalRadius - distance;

		// プレイヤーのみを押し出し（敵は動かない）
		Vector3 pushVector = direction * overlap;

		// プレイヤーを敵から離す方向に押し出し
		player->SetTranslation(playerPos - pushVector);

		// プレイヤーの速度を調整（敵の方向への移動を止める）
		Vector3 playerVelocity = player->GetVelocity();

		// 敵の方向への速度成分を除去
		float dotProduct = playerVelocity.Dot(-direction);
		if (dotProduct > 0) {
			// 敵の方向に向かう速度成分を除去
			Vector3 projectedVelocity = -direction * dotProduct;
			playerVelocity -= projectedVelocity;
			player->SetVelocity(playerVelocity);
		}

		// 敵の接近を一時的に停止
		velocity_ *= 0.1f;

		// トランスフォーム更新
		transform_.UpdateMatrix();
	}
}

void Enemy::Approach()
{
	if (player_ == nullptr) return;

	Vector3 targetDirection = (player_->GetCenterPosition() - GetCenterPosition());
	float distanceToPlayer = targetDirection.Length();

	// プレイヤーとの距離が短すぎる場合は接近を停止
	if (distanceToPlayer < shortDistance_) {
		velocity_ *= 0.9f; // 速度を減衰
		return;
	}

	// 通常の接近処理
	targetDirection = targetDirection.Normalize();
	Vector3 approachVelocity = targetDirection * approachSpeed_;

	// 現在の速度と合成
	velocity_ = velocity_ * 0.8f + approachVelocity * 0.2f;

	// 速度制限
	if (velocity_.Length() > maxSpeed_) {
		velocity_ = velocity_.Normalize() * maxSpeed_;
	}

	// 位置更新
	BaseObject::SetWorldPosition(GetCenterPosition() + velocity_);
}