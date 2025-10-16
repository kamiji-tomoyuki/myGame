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
	BaseObject::SetWorldPosition(Vector3{ 0.0f,2.0f,15.0f });

	float size = 1.0f;

	Collider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kEnemy));
	Collider::SetRadius(size);

	// --- モデルの初期化 ---
	obj3d_ = std::make_unique<Object3d>();
	obj3d_->Initialize("Enemy/enemyBody.obj");
	obj3d_->SetSize({ size,size,size });
	obj3d_->SetRotation({ 0.0f,1.57f * 2.0f,0.0f });
	BaseObject::SetRotation(obj3d_->GetRotation());
	BaseObject::SetScale(obj3d_->GetSize());

	// --- 各ステータスの初期値設定 ---
	originalRotation_ = obj3d_->GetRotation();
	behavior_ = Behavior::kRoot;
	chargeTimer_ = 0;
	maxChargeCount_ = 1;  // 通常時は1回
}

void Enemy::Update(Player* player, const ViewProjection &vp)
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
		// 行動状態の更新
		UpdateBehavior();
	}

	obj3d_->Update(BaseObject::GetWorldTransform(), vp);
}

void Enemy::UpdateBehavior()
{
	switch (behavior_) {
	case Behavior::kRoot:
		// 通常時は接近処理
		Approach();

		// 通常の回転に戻す処理
		RecoverRotation();

		// 突進攻撃の準備タイマー更新
		chargeTimer_++;
		if (chargeTimer_ >= kChargePreparationTime_) {
			// プレイヤーとの距離をチェック
			if (player_ != nullptr) {
				float distanceToPlayer = (player_->GetCenterPosition() - GetCenterPosition()).Length();
				if (distanceToPlayer <= kChargeRange_) {
					StartCharge();
				}
				else {
					// 距離が遠すぎる場合はタイマーをリセット
					chargeTimer_ = 0;
				}
			}
		}
		break;

	case Behavior::kAttack:
		UpdateCharge();
		break;

	case Behavior::kCooldown:
		UpdateCooldown();
		break;
	}
}

void Enemy::StartCharge()
{
	behavior_ = Behavior::kAttack;
	chargeDuration_ = 0;
	chargeCount_++;

	// HPが半分以下の場合は最大突進回数を2回に設定
	if (HP_ <= kMaxHP_ / 2) {
		maxChargeCount_ = 2;
	}

	// プレイヤーへの方向を計算
	if (player_ != nullptr) {
		Vector3 targetPos = player_->GetCenterPosition();
		Vector3 currentPos = GetCenterPosition();
		chargeDirection_ = (targetPos - currentPos).Normalize();
		chargeStartPos_ = currentPos;

		// プレイヤーの方向を向く
		float targetRotationY = std::atan2(chargeDirection_.x, chargeDirection_.z);
		BaseObject::SetRotationY(targetRotationY);
		obj3d_->SetRotation(BaseObject::GetWorldRotation());
	}
}

void Enemy::UpdateCharge()
{
	chargeDuration_++;

	// 突進移動
	Vector3 chargeVelocity = chargeDirection_ * kChargeSpeed_;
	BaseObject::SetWorldPosition(GetCenterPosition() + chargeVelocity);

	// 突進時間が終了した場合
	if (chargeDuration_ >= kChargeDuration_) {
		EndCharge();
	}
}

void Enemy::EndCharge()
{
	// まだ突進回数が残っている場合
	if (chargeCount_ < maxChargeCount_) {
		// 次の突進の準備
		behavior_ = Behavior::kRoot;
		chargeTimer_ = kChargePreparationTime_ - 30; // 短い間隔で次の突進
		chargeDuration_ = 0;
	}
	else {
		// 全ての突進が終了した場合はクールダウン開始
		StartCooldown();
	}
}

void Enemy::StartCooldown()
{
	behavior_ = Behavior::kCooldown;
	cooldownTimer_ = 0;
	chargeCount_ = 0;  // 突進回数をリセット
}

void Enemy::UpdateCooldown()
{
	cooldownTimer_++;

	// 通常の接近処理を継続
	Approach();
	RecoverRotation();

	// クールダウン時間が終了した場合
	if (cooldownTimer_ >= kCooldownTime_) {
		behavior_ = Behavior::kRoot;
		chargeTimer_ = 0;
		cooldownTimer_ = 0;
	}
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

	// ラッシュ攻撃中は突進攻撃を中断
	if (behavior_ == Behavior::kAttack) {
		behavior_ = Behavior::kCooldown;
		cooldownTimer_ = 0;
		chargeCount_ = 0;
	}

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

	// 現在のBaseObjectの回転を取得（Y軸回転が含まれる）
	Vector3 currentRotation = BaseObject::GetWorldRotation();

	// 後ろに傾く処理
	float tiltAmount = sin(static_cast<float>(rushKnockbackTimer_) * 0.1f) * maxTiltAngle_;

	// 元の回転にY軸回転を保持したまま、X軸の傾きを追加
	Vector3 objRotation = currentRotation;
	objRotation.x = originalRotation_.x + tiltAmount; // X軸で後ろに傾く

	obj3d_->SetRotation(objRotation);

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
	Vector3 baseRotation = BaseObject::GetWorldRotation();

	// X軸とZ軸のみを元に戻す（Y軸はApproachで管理）
	float lerpFactor = 0.05f;
	Vector3 newRotation = {
		currentRotation.x + (originalRotation_.x - currentRotation.x) * lerpFactor,
		baseRotation.y, // Y軸回転はBaseObjectから取得
		currentRotation.z + (originalRotation_.z - currentRotation.z) * lerpFactor
	};

	obj3d_->SetRotation(newRotation);
}

void Enemy::Draw(const ViewProjection& viewProjection)
{
	obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection);
}

void Enemy::DrawAnimation(const ViewProjection& viewProjection)
{

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

		// 突進攻撃中の場合はより強い押し出し処理
		if (behavior_ == Behavior::kAttack) {
			// プレイヤーに大きなダメージやノックバックを与える処理をここに追加可能
			overlap *= 2.0f; // 突進時は押し出し力を強化
		}

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

		// 敵の接近を一時的に停止（突進中は除く）
		if (behavior_ != Behavior::kAttack) {
			velocity_ *= 0.1f;
		}

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
	}
	else {
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

	// プレイヤーの方向を向く処理（距離に関係なく実行）
	if (distanceToPlayer > 0.0001f) { // より小さな閾値を使用
		// 正規化された方向ベクトルを取得
		Vector3 normalizedDirection = targetDirection.Normalize();

		// プレイヤーの方向を向く（Y軸回転を計算）
		float targetRotationY = std::atan2(normalizedDirection.x, normalizedDirection.z);

		// 現在の回転を取得
		Vector3 currentRotation = GetCenterRotation();
		float currentRotationY = currentRotation.y;

		// 角度差を計算し、最短経路で回転するよう正規化
		float angleDiff = targetRotationY - currentRotationY;

		// 角度を-π〜πの範囲に正規化（最短経路での回転）
		const float PI = 3.14159265359f;
		while (angleDiff > PI) {
			angleDiff -= 2.0f * PI;
		}
		while (angleDiff < -PI) {
			angleDiff += 2.0f * PI;
		}

		// 新しい回転角度を線形補間で計算
		float lerpFactor = 0.1f; // 補間係数を調整
		float newRotationY = currentRotationY + angleDiff * lerpFactor;

		// 回転を設定
		if (!isBeingRushed_) {
			// 通常時はそのまま回転を適用
			BaseObject::SetRotation(Vector3(currentRotation.x, newRotationY, currentRotation.z));
			obj3d_->SetRotation(BaseObject::GetWorldRotation());
		}
		else {
			// ラッシュ攻撃中でもY軸回転は更新（ただし傾きは UpdateRushKnockback で管理）
			Vector3 newRotation = Vector3(currentRotation.x, newRotationY, currentRotation.z);
			BaseObject::SetRotation(newRotation);
			// obj3d_の回転はUpdateRushKnockbackで設定されるため、ここでは設定しない
		}
	}
}