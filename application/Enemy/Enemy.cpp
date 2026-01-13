#include "Enemy.h"

#include "CollisionTypeIdDef.h"
#include "Player.h"
#include "Arm/PlayerArm.h"
#include "EnemyAttackManager.h"
#include "EnemyAttackMelee.h"
#include "EnemyAttackRanged.h"

#include "myMath.h"
#include <Easing.h>

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

	// --- 攻撃管理の初期化 ---
	attackManager_ = std::make_unique<EnemyAttackManager>();
	attackManager_->Initialize();
}

void Enemy::Update(Player* player, const ViewProjection& vp)
{
	// ゲーム状態に応じた更新処理
	switch (gameState_) {
	case GameState::kPlaying:
		// ゲームプレイ中の処理
		player_ = player;
		vp_ = &vp;

		// プレイヤーのラッシュ攻撃状態をチェック
		CheckPlayerRushStatus();

		// ラッシュ攻撃を受けている時の処理
		if (isBeingRushed_) {
			UpdateRushKnockback();
		}
		else {
			// 通常時は接近処理
			Approach();
			RecoverRotation();

			// 攻撃管理の更新
			attackManager_->Update(this, player);
		}
		break;

	case GameState::kGameOver:
		// ゲームオーバー演出
		UpdateGameOverEffect();
		break;

	case GameState::kGameClear:
		// ゲームクリア演出
		UpdateGameClearEffect();
		break;
	}

	BaseObject::Update();
	obj3d_->Update(BaseObject::GetWorldTransform(), vp);

	// 攻撃管理のビュープロジェクション更新
	if (vp_ != nullptr && attackManager_) {
		if (auto* rangedAttack = attackManager_->GetRangedAttack()) {
			rangedAttack->UpdateViewProjection(vp);
		}
	}
}

void Enemy::UpdateGameOverEffect()
{
	// ゲームオーバー時はジャンプし続ける演出

	BaseObject::SetScale(Vector3(1.5f, 1.5f, 1.5f));

	const float kJumpCycle = 40.0f;
	const float kJumpHeight = 2.0f;  // ジャンプの高さ

	const Vector3 jumpEndPos = { 0.0f, 2.0f, 4.0f };

	// 現在のジャンプフェーズを計算(0.0~1.0)
	float jumpPhase = fmod(fallTimer_, kJumpCycle) / kJumpCycle;

	// sin波を使って滑らかなジャンプモーションを作成
	float jumpOffset = sin(jumpPhase * 3.14159265f) * kJumpHeight;

	// 地面の位置にジャンプオフセットを加える
	Vector3 currentPos = jumpEndPos;
	currentPos.y += jumpOffset;

	BaseObject::SetWorldPosition(currentPos);

	// 回転のアニメーション
	Vector3 currentRotation = obj3d_->GetRotation();
	currentRotation.y += 0.05f;  // ゆっくり回転
	obj3d_->SetRotation(currentRotation);

	// タイマーを進める
	fallTimer_++;

	// タイマーがオーバーフローしないようにリセット
	if (fallTimer_ >= kJumpCycle * 1000.0f) {
		fallTimer_ = 0.0f;
	}
}

void Enemy::UpdateGameClearEffect()
{
	const float kWaitTime = 1.2f * 60.0f;

	clearEffectTimer_++;

	// 待機時間中は何もしない
	if (clearEffectTimer_ < kWaitTime) {
		return;
	}

	// 待機後、回転しながら縮小
	// 回転速度を上げる
	Vector3 currentRotation = obj3d_->GetRotation();
	currentRotation.y += 0.5f;  // ゲームオーバー時のプレイヤーと同じ速度
	obj3d_->SetRotation(currentRotation);
	BaseObject::SetRotation(currentRotation);

	// 縮小処理
	Vector3 currentScale = BaseObject::GetWorldSize();
	if (currentScale.x >= 0.0f) {
		BaseObject::SetScale(Vector3(
			currentScale.x - 0.02f,
			currentScale.y - 0.02f,
			currentScale.z - 0.02f));
	}
	else {
		isAlive_ = false;
	}
}

void Enemy::UpdateStartEffect()
{
	BaseObject::Update();

	if (fallTimer_ < kFallDuration_) {
		// イージングを使った落下演出
		Vector3 currentPos = EaseOutBounce<Vector3>(
			fallStartPos_,
			fallEndPos_,
			fallTimer_,
			kFallDuration_
		);

		BaseObject::SetWorldPosition(currentPos);

		// タイマーを進める
		fallTimer_++;

		// 落下中は回転させる演出を追加 (オプション)
		float rotationSpeed = 0.1f;
		Vector3 currentRotation = obj3d_->GetRotation();
		currentRotation.y += rotationSpeed * (1.0f - (fallTimer_ / kFallDuration_));
		obj3d_->SetRotation(currentRotation);
	}
	else {
		// 落下完了後は最終位置に固定
		BaseObject::SetWorldPosition(fallEndPos_);

		// 落下完了フラグ
		isFallComplete_ = true;
	}
}

void Enemy::CheckPlayerRushStatus()
{
	if (player_ == nullptr) return;

	bool wasBeingRushed = isBeingRushed_;

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
	//------------------------------------------
	if (HP_ < 0) {
		isAlive_ = false;
	}
	//------------------------------------------

	isBeingRushed_ = true;
	rushKnockbackTimer_ = 0;

	// ラッシュ攻撃中は攻撃を中断
	if (attackManager_) {
		attackManager_->InterruptByRush();
	}

	// プレイヤーから敵への方向を計算(後退方向)
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

	// 一定時間経過したら自動的にラッシュ状態を解除
	// プレイヤーのラッシュ攻撃時間は120フレーム (kRushDuration)なので、
	// 余裕を持って150フレームで解除
	static constexpr uint32_t kMaxRushKnockbackDuration = 150;
	if (rushKnockbackTimer_ >= kMaxRushKnockbackDuration) {
		EndRushKnockback();
		return;
	}

	// 現在のBaseObjectの回転を取得(Y軸回転が含まれる)
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

	// X軸とZ軸のみを元に戻す(Y軸はApproachで管理)
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

	// 攻撃の描画
	if (attackManager_) {
		if (auto* rangedAttack = attackManager_->GetRangedAttack()) {
			rangedAttack->Draw(viewProjection);
		}
	}
}

void Enemy::DrawAnimation(const ViewProjection& viewProjection)
{

}

void Enemy::DrawParticle(const ViewProjection& viewProjection)
{
	// 攻撃のパーティクル描画
	if (attackManager_) {
		if (auto* meleeAttack = attackManager_->GetMeleeAttack()) {
			meleeAttack->DrawTrailEffect();
		}
	}
}

void Enemy::ImGui()
{
	// ImGui処理
}

void Enemy::OnCollision(Collider* other)
{
	uint32_t typeID = other->GetTypeID();

	// プレイヤーとの衝突処理
	if (typeID == static_cast<uint32_t>(CollisionTypeIdDef::kPlayer)) {
		Player* player = static_cast<Player*>(other);

		//------------------------------------------
		if (HP_ < 0) {
			isAlive_ = false;
		}
		//------------------------------------------

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

		//------------------------------------------
		if (HP_ < 0) {
			isAlive_ = false;
		}
		//------------------------------------------

		// ラッシュ攻撃中の場合
		if (arm->GetBehavior() == PlayerArm::Behavior::kRush) {

			// まだラッシュ攻撃を受け始めていない場合は初期化
			if (!isBeingRushed_) {
				isBeingRushed_ = true;
				StartRushKnockback();
			}
			// すでにラッシュ中の場合はタイマーを更新して継続
			else {
				// タイマーを少し戻すことで、ラッシュが続いている間は解除されない
				if (rushKnockbackTimer_ > 30) {
					rushKnockbackTimer_ = 30;
				}
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

		// 攻撃中の場合はより強い押し出し処理
		if (attackManager_ && attackManager_->IsAttacking()) {
			overlap *= 2.0f; // 攻撃時は押し出し力を強化
		}

		// プレイヤーのみを押し出し(敵は動かない)
		Vector3 pushVector = direction * overlap;

		// プレイヤーを敵から離す方向に押し出し
		player->SetTranslation(playerPos - pushVector);

		// プレイヤーの速度を調整(敵の方向への移動を止める)
		Vector3 playerVelocity = player->GetVelocity();

		// 敵の方向への速度成分を除去
		float dotProduct = playerVelocity.Dot(-direction);
		if (dotProduct > 0) {
			// 敵の方向に向かう速度成分を除去
			Vector3 projectedVelocity = -direction * dotProduct;
			playerVelocity -= projectedVelocity;
			player->SetVelocity(playerVelocity);
		}

		// 敵の接近を一時的に停止(攻撃中は除く)
		if (!attackManager_ || !attackManager_->IsAttacking()) {
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

bool Enemy::IsAttacking() const
{
	if (attackManager_) {
		return attackManager_->IsAttacking();
	}
	return false;
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

	// プレイヤーの方向を向く処理(距離に関係なく実行)
	if (distanceToPlayer > 0.0001f) { // より小さな閾値を使用
		// 正規化された方向ベクトルを取得
		Vector3 normalizedDirection = targetDirection.Normalize();

		// プレイヤーの方向を向く(Y軸回転を計算)
		float targetRotationY = std::atan2(normalizedDirection.x, normalizedDirection.z);

		// 現在の回転を取得
		Vector3 currentRotation = GetCenterRotation();
		float currentRotationY = currentRotation.y;

		// 角度差を計算し、最短経路で回転するよう正規化
		float angleDiff = targetRotationY - currentRotationY;

		// 角度を-π~πの範囲に正規化(最短経路での回転)
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
			// ラッシュ攻撃中でもY軸回転は更新(ただし傾きは UpdateRushKnockback で管理)
			Vector3 newRotation = Vector3(currentRotation.x, newRotationY, currentRotation.z);
			BaseObject::SetRotation(newRotation);
			// obj3d_の回転はUpdateRushKnockbackで設定されるため、ここでは設定しない
		}
	}
}