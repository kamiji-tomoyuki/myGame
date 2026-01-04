#include "Enemy.h"

#include "CollisionTypeIdDef.h"
#include "Player.h"
#include "Arm/PlayerArm.h"

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
	behavior_ = Behavior::kRoot;
	chargeTimer_ = 0;
	maxChargeCount_ = 1;  // 通常時は1回

	// 軌跡パーティクルの初期化
	trailEffect_ = std::make_unique<ParticleEmitter>();
	trailEffect_->Initialize("enemyTrail", "debug/plane.obj");

	Vector3 initialFootPos = BaseObject::GetWorldPosition();
	initialFootPos.y += kFootOffsetY_;
	lastTrailPosition_ = initialFootPos;

	originalRotation_ = obj3d_->GetRotation();
	behavior_ = Behavior::kRoot;
	chargeTimer_ = 0;
	maxChargeCount_ = 1;
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
			// 行動状態の更新
			UpdateBehavior();
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

	// 軌跡パーティクルの更新
	trailEffect_->UpdateOnce(vp);

	// 遠距離攻撃のモデル更新
	if (vp_ != nullptr) {
		for (auto& attack : rangedAttacks_) {
			if (attack.warningCircle) {
				WorldTransform warningTransform;
				warningTransform.Initialize();
				warningTransform.translation_ = attack.position;
				warningTransform.rotation_ = attack.warningCircle->GetRotation();
				warningTransform.scale_ = attack.warningCircle->GetSize();
				warningTransform.UpdateMatrix();
				attack.warningCircle->Update(warningTransform, vp);
			}

			if (attack.spike) {
				WorldTransform spikeTransform;
				spikeTransform.Initialize();
				spikeTransform.translation_ = attack.position;
				spikeTransform.translation_.y += attack.spikeHeight * 0.5f;
				spikeTransform.scale_ = attack.spike->GetSize();
				spikeTransform.UpdateMatrix();
				attack.spike->Update(spikeTransform, vp);
			}
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

		// 落下中は回転させる演出を追加(オプション)
		float rotationSpeed = 0.1f;
		Vector3 currentRotation = obj3d_->GetRotation();
		currentRotation.y += rotationSpeed * (1.0f - (fallTimer_ / kFallDuration_));
		obj3d_->SetRotation(currentRotation);
	}
	else {
		// 落下完了後は最終位置に固定
		BaseObject::SetWorldPosition(fallEndPos_);

		// 落下完了フラグ(ヘッダーに追加が必要)
		// bool isFallComplete_ = true;
		isFallComplete_ = true;
	}
}

void Enemy::UpdateBehavior()
{
	switch (behavior_) {
	case Behavior::kRoot:
		// 通常時は接近処理
		Approach();

		// 通常の回転に戻す処理
		RecoverRotation();

		// 攻撃準備タイマー更新
		chargeTimer_++;
		if (chargeTimer_ >= kChargePreparationTime_) {
			// プレイヤーとの距離をチェック
			if (player_ != nullptr) {
				float distanceToPlayer = (player_->GetCenterPosition() - GetCenterPosition()).Length();

				// 近距離(15.0f以内)なら突進攻撃
				if (distanceToPlayer <= kChargeRange_) {
					StartCharge();
				}
				else {
				// 遠距離(15.0f超)なら遠距離攻撃
					StartRangedAttack();
				}
			}
		}
		break;

	case Behavior::kAttack:
		UpdateCharge();
		break;

	case Behavior::kRangedAttack:
		UpdateRangedAttack();
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

	// 軌跡パーティクルの発生処理
	UpdateTrailEffect();

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

void Enemy::StartRangedAttack()
{
	behavior_ = Behavior::kRangedAttack;
	rangedAttackTimer_ = 0;
	rangedAttackPhase_ = 0;
	rangedAttacks_.clear();

	// プレイヤーの方向を向く
	if (player_ != nullptr) {
		Vector3 targetPos = player_->GetCenterPosition();
		Vector3 currentPos = GetCenterPosition();
		Vector3 direction = (targetPos - currentPos).Normalize();

		float targetRotationY = std::atan2(direction.x, direction.z);
		BaseObject::SetRotationY(targetRotationY);
		obj3d_->SetRotation(BaseObject::GetWorldRotation());
	}

	// 移動を停止（硬直状態）
	velocity_ = Vector3(0.0f, 0.0f, 0.0f);
}

void Enemy::UpdateRangedAttack()
{
	rangedAttackTimer_++;

	// 硬直状態を維持（移動しない）
	velocity_ = Vector3(0.0f, 0.0f, 0.0f);

	// 予備動作フェーズ
	if (rangedAttackTimer_ < kRangedAttackPreparationTime_) {
		// 前傾姿勢のアニメーション
		float tiltProgress = static_cast<float>(rangedAttackTimer_) / kRangedAttackPreparationTime_;
		float tiltAmount = tiltProgress * kRangedAttackTiltAngle_;

		Vector3 currentRotation = BaseObject::GetWorldRotation();
		Vector3 objRotation = currentRotation;
		objRotation.x = originalRotation_.x + tiltAmount;
		obj3d_->SetRotation(objRotation);
	}
	// ジオグリフ生成フェーズ
	else {
		uint32_t attackPhaseTimer = rangedAttackTimer_ - kRangedAttackPreparationTime_;

		// 各攻撃の生成タイミング（間隔をあけて3つ生成）
		if (attackPhaseTimer % kRangedAttackInterval_ == 0 && rangedAttackPhase_ < kRangedAttackCount_) {
			RangedAttackInstance newAttack;

			// プレイヤーの現在位置を取得（各トゲごとに更新）
			Vector3 currentPlayerPos = Vector3(0.0f, 0.0f, 0.0f);
			if (player_ != nullptr) {
				currentPlayerPos = player_->GetCenterPosition();
			}

			// プレイヤーの足元に出現
			newAttack.position = currentPlayerPos;
			newAttack.position.y = 0.1f; // 地面の高さ
			newAttack.isWarningActive = true;

			// 警告円の初期化（個別に新しいオブジェクトを作成）
			newAttack.warningCircle = std::make_unique<Object3d>();
			newAttack.warningCircle->Initialize("debug/ground.obj");
			newAttack.warningCircle->SetSize({ kWarningCircleRadius_, 1.0f, kWarningCircleRadius_ });
			newAttack.warningCircle->SetRotation({ 1.57f, 0.0f, 0.0f }); // X軸90度回転で地面に

			// トゲモデルの初期化（個別に新しいオブジェクトを作成）
			newAttack.spike = std::make_unique<Object3d>();
			newAttack.spike->Initialize("Enemy/Cube.obj");
			newAttack.spike->SetSize({ 1.0f, 0.0f, 1.0f }); // 初期は高さ0

			rangedAttacks_.push_back(std::move(newAttack));
			rangedAttackPhase_++;
		}
	}

	// 既存の攻撃インスタンスを更新
	UpdateRangedAttackInstances();

	// 全ての攻撃が終了したかチェック
	bool allFinished = rangedAttackPhase_ >= kRangedAttackCount_;
	if (allFinished) {
		bool anyActive = false;
		for (const auto& attack : rangedAttacks_) {
			if (attack.isWarningActive || attack.isSpikeActive) {
				anyActive = true;
				break;
			}
		}
		if (!anyActive) {
			EndRangedAttack();
		}
	}
}

void Enemy::UpdateRangedAttackInstances()
{
	for (auto& attack : rangedAttacks_) {
		// 警告円の更新
		if (attack.isWarningActive) {
			attack.warningTimer++;

			if (attack.warningTimer >= kWarningDuration_) {
				attack.isWarningActive = false;
				attack.isSpikeActive = true;
				attack.spikeTimer = 0;
			}
		}

		// トゲの更新
		if (attack.isSpikeActive) {
			attack.spikeTimer++;

			// トゲの上昇アニメーション
			if (attack.spikeTimer < kSpikeRiseDuration_) {
				// 上昇フェーズ
				float riseProgress = static_cast<float>(attack.spikeTimer) / kSpikeRiseDuration_;
				attack.spikeHeight = riseProgress * kSpikeMaxHeight_;
			}
			// トゲの持続
			else if (attack.spikeTimer < kSpikeRiseDuration_ + kSpikeHoldDuration_) {
				// 最大高さを維持
				attack.spikeHeight = kSpikeMaxHeight_;
			}
			// トゲの下降アニメーション
			else if (attack.spikeTimer < kSpikeRiseDuration_ + kSpikeHoldDuration_ + kSpikeFallDuration_) {
				// 下降フェーズ
				uint32_t fallTimer = attack.spikeTimer - (kSpikeRiseDuration_ + kSpikeHoldDuration_);
				float fallProgress = static_cast<float>(fallTimer) / kSpikeFallDuration_;
				attack.spikeHeight = kSpikeMaxHeight_ * (1.0f - fallProgress);
			}
			else {
				// 完全に引っ込んだら非アクティブに
				attack.isSpikeActive = false;
				attack.spikeHeight = 0.0f;
			}

			// トゲのサイズ更新
			attack.spike->SetSize({ 1.0f, attack.spikeHeight, 1.0f });
		}
	}

	// 当たり判定チェック
	CheckRangedAttackCollision();
}

void Enemy::EndRangedAttack()
{
	// 姿勢を元に戻す
	Vector3 currentRotation = BaseObject::GetWorldRotation();
	currentRotation.x = originalRotation_.x;
	BaseObject::SetRotation(currentRotation);
	obj3d_->SetRotation(currentRotation);

	// クールダウンに移行
	StartCooldown();

	// 攻撃データをクリア
	rangedAttacks_.clear();
	rangedAttackPhase_ = 0;
	rangedAttackTimer_ = 0;
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

void Enemy::CheckRangedAttackCollision()
{
	if (player_ == nullptr) return;

	Vector3 playerPos = player_->GetCenterPosition();

	for (const auto& attack : rangedAttacks_) {
		if (!attack.isSpikeActive) continue;

		// トゲが十分に伸びている場合のみ判定
		if (attack.spikeHeight < kSpikeMaxHeight_ * 0.5f) continue;

		// 円形の当たり判定
		float distanceXZ = std::sqrt(
			std::pow(playerPos.x - attack.position.x, 2.0f) +
			std::pow(playerPos.z - attack.position.z, 2.0f)
		);

		// プレイヤーがトゲの範囲内にいる場合
		if (distanceXZ <= kWarningCircleRadius_ &&
			playerPos.y <= attack.position.y + kSpikeMaxHeight_) {
			// プレイヤーにダメージを与える処理
			// プレイヤーが回避中でない場合のみダメージ
			if (!player_->IsDodging()) {
				//player_->TakeDamage(attack.position);
			}
		}
	}
}

void Enemy::CheckPlayerRushStatus()
{
	if (player_ == nullptr) return;

	bool wasBeingRushed = isBeingRushed_;

	// プレイヤーの腕の状態をチェックしてラッシュ判定
	bool playerIsRushing = false;

	// プレイヤーの両腕をチェックして、どちらかがラッシュ状態ならtrue
	// (Playerクラスに腕へのアクセサがない場合は、OnCollisionでの判定のみに依存)
	// ここでは、OnCollisionで設定された isBeingRushed_ の状態を維持し、
	// 一定時間経過で自動的に解除する方式に変更します

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

	// ラッシュ攻撃中は突進攻撃を中断
	if (behavior_ == Behavior::kAttack) {
		behavior_ = Behavior::kCooldown;
		cooldownTimer_ = 0;
		chargeCount_ = 0;
	}

	// 遠距離攻撃中も中断
	if (behavior_ == Behavior::kRangedAttack) {
		EndRangedAttack();
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

void Enemy::UpdateTrailEffect()
{
	Vector3 currentPos = GetCenterPosition();

	// 足元の位置を計算
	Vector3 footPosition = currentPos;
	footPosition.y += kFootOffsetY_;

	float distanceMoved = (footPosition - lastTrailPosition_).Length();

	// 一定距離移動したらパーティクルを発生
	if (distanceMoved >= trailEmitDistance_) {
		// 移動している場合のみパーティクルを発生
		if (velocity_.Length() > 0.01f) {
			trailEffect_->SetPosition(footPosition);
			trailEffect_->SetActive(false);
		}
		lastTrailPosition_ = footPosition;
	}
}

void Enemy::Draw(const ViewProjection& viewProjection)
{
	obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection);
	DrawRangedAttack(viewProjection);
}

void Enemy::DrawAnimation(const ViewProjection& viewProjection)
{

}

void Enemy::DrawParticle(const ViewProjection& viewProjection)
{
	trailEffect_->Draw(Normal);
}

void Enemy::DrawRangedAttack(const ViewProjection& viewProjection)
{
	for (const auto& attack : rangedAttacks_) {
		if (attack.isWarningActive && attack.warningCircle) {
			WorldTransform warningTransform;
			warningTransform.Initialize();
			warningTransform.translation_ = attack.position;
			warningTransform.rotation_ = attack.warningCircle->GetRotation();
			warningTransform.scale_ = attack.warningCircle->GetSize();
			warningTransform.UpdateMatrix();

			// 赤色で描画
			ObjColor redColor;
			redColor.Initialize();
			redColor.SetColor(Vector4(1.0f, 0.0f, 0.0f, 0.6f));
			redColor.TransferMatrix();

			//attack.warningCircle->Draw(warningTransform, viewProjection, &redColor);
		}

		if (attack.isSpikeActive && attack.spike) {
			WorldTransform spikeTransform;
			spikeTransform.Initialize();
			spikeTransform.translation_ = attack.position;
			spikeTransform.translation_.y += attack.spikeHeight * 0.5f;
			spikeTransform.scale_ = attack.spike->GetSize();
			spikeTransform.UpdateMatrix();

			attack.spike->Draw(spikeTransform, viewProjection);
		}
	}
}

void Enemy::ImGui()
{
	trailEffect_->imgui();
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

		// 突進攻撃中の場合はより強い押し出し処理
		if (behavior_ == Behavior::kAttack) {
			// プレイヤーに大きなダメージやノックバックを与える処理をここに追加可能
			overlap *= 2.0f; // 突進時は押し出し力を強化
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

		// 敵の接近を一時的に停止(突進中は除く)
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

	UpdateTrailEffect();
}