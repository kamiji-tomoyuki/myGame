#include "Enemy.h"

#include "CollisionTypeIdDef.h"
#include "Player.h"
#include "Arm/PlayerArm.h"

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
	obj3d_->Initialize("Enemy/enemy.gltf");
	obj3d_->SetRotation({ 0.0f,1.57f * 2.0f,0.0f });
	BaseObject::SetRotation(obj3d_->GetRotation());

	// --- 各ステータスの初期値設定 ---
	originalRotation_ = obj3d_->GetRotation();
}

void Enemy::Update(Player* player)
{
	player_ = player;
	BaseObject::Update();

	// プレイヤーのラッシュ攻撃状態をチェック
	CheckPlayerRushStatus();

	// ラッシュ攻撃を受けている時の処理
	if (isBeingRushed_) {
		UpdateRushKnockback();
	}
	else {
		// 通常時は接近処理
		Approach();

		// 通常の回転に戻す処理
		RecoverRotation();
	}

	// アニメーションの再生
	obj3d_->UpdateAnimation(true);
}

void Enemy::CheckPlayerRushStatus()
{
	if (player_ == nullptr) return;

	bool wasBeingRushed = isBeingRushed_;

	// プレイヤーの腕の状態をチェックしてラッシュ判定
	bool playerIsRushing = false;

	// ラッシュ攻撃が開始された瞬間の処理
	if (!wasBeingRushed && isBeingRushed_) {
		StartRushKnockback();
	}

	// ラッシュ攻撃が終了した瞬間の処理
	if (wasBeingRushed && !isBeingRushed_) {
		EndRushKnockback();
	}
}

void Enemy::StartRushKnockback()
{
	isBeingRushed_ = true;
	rushKnockbackTimer_ = 0;

	// プレイヤーから敵への方向を計算（後退方向）
	if (player_ != nullptr) {
		Vector3 direction = GetCenterPosition() - player_->GetCenterPosition();
		if (direction.Length() > 0.0f) {
			knockbackDirection_ = direction.Normalize();
		}
		else {
			knockbackDirection_ = Vector3(0.0f, 0.0f, 1.0f); // デフォルト方向
		}
	}
}

void Enemy::UpdateRushKnockback()
{
	rushKnockbackTimer_++;

	// 後ろに傾く処理
	float tiltAmount = sin(static_cast<float>(rushKnockbackTimer_) * 0.1f) * maxTiltAngle_;
	Vector3 currentRotation = originalRotation_;
	currentRotation.x += tiltAmount; // X軸で後ろに傾く

	obj3d_->SetRotation(currentRotation);
	BaseObject::SetRotation(currentRotation);

	// 後退処理
	Vector3 knockbackVelocity = knockbackDirection_ * knockbackSpeed_;
	Vector3 currentPos = GetCenterPosition();
	BaseObject::SetWorldPosition(currentPos + knockbackVelocity);

	// 速度を減衰させて段々と後退速度を落とす
	knockbackSpeed_ *= knockbackDecay_;
	if (knockbackSpeed_ < minKnockbackSpeed_) {
		knockbackSpeed_ = minKnockbackSpeed_;
	}
}

void Enemy::EndRushKnockback()
{
	isBeingRushed_ = false;
	rushKnockbackTimer_ = 0;
	knockbackSpeed_ = initialKnockbackSpeed_; // 初期値に戻す
}

void Enemy::RecoverRotation()
{
	// 通常の回転に徐々に戻す
	Vector3 currentRotation = obj3d_->GetRotation();
	Vector3 targetRotation = originalRotation_;

	// 線形補間で滑らかに戻す
	float lerpFactor = 0.05f;
	Vector3 newRotation = {
		currentRotation.x + (targetRotation.x - currentRotation.x) * lerpFactor,
		currentRotation.y + (targetRotation.y - currentRotation.y) * lerpFactor,
		currentRotation.z + (targetRotation.z - currentRotation.z) * lerpFactor
	};

	obj3d_->SetRotation(newRotation);
	BaseObject::SetRotation(newRotation);
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

	// プレイヤーの腕との衝突処理
	if (typeID == static_cast<uint32_t>(CollisionTypeIdDef::kPArm)) {
		PlayerArm* arm = static_cast<PlayerArm*>(other);

		// ラッシュ攻撃中の場合
		if (arm->GetBehavior() == PlayerArm::Behavior::kRush) {
			isBeingRushed_ = true;

			// まだラッシュ攻撃を受け始めていない場合は初期化
			if (rushKnockbackTimer_ == 0) {
				StartRushKnockback();
			}
		}
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

void Enemy::TakeDamage(uint32_t damage)
{
	HP_ -= damage;
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

	// プレイヤーの方向を向く（Y軸回転を計算）
	float targetRotationY = std::atan2(targetDirection.x, targetDirection.z);

	// 現在の回転を取得
	Vector3 currentRotation = GetCenterRotation();

	// Y軸回転を補間して滑らかに回転（lerp係数は調整可能）
	float lerpFactor = 0.1f;
	float currentRotationY = currentRotation.y;

	// 角度差を計算（-π～πの範囲で正規化）
	float angleDiff = targetRotationY - currentRotationY;
	if (angleDiff > 3.14159f) {
		angleDiff -= 2.0f * 3.14159f;
	}
	else if (angleDiff < -3.14159f) {
		angleDiff += 2.0f * 3.14159f;
	}

	// 新しい回転角度を計算
	float newRotationY = currentRotationY + angleDiff * lerpFactor;

	// 回転を設定（ラッシュ攻撃中でない場合のみ）
	if (!isBeingRushed_) {
		BaseObject::SetRotation(Vector3(currentRotation.x, newRotationY, currentRotation.z));
		obj3d_->SetRotation(BaseObject::GetWorldRotation());
	}

	// 位置更新
	BaseObject::SetWorldPosition(GetCenterPosition() + velocity_);
}