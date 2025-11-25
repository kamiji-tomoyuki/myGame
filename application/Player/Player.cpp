#include "Player.h"
#include "Input.h"

#include "FollowCamera.h"
#include "CollisionTypeIdDef.h"

#include "myMath.h"
#include <Enemy.h>
#include <random>

Player::Player()
{
	serialNumber_ = nextSerialNumber_;
	nextSerialNumber_++;
}

void Player::Init()
{
	BaseObject::Init();
	Collider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayer));

	// --- ステージマネージャー ---
	stageManager_ = StageManager::GetInstance();
	stageManager_->Initialize();

	Vector3 initialPos = { 0.0f, 0.0f, -15.0f };
	BaseObject::SetWorldPosition(initialPos);
	BaseObject::SetScale({ 0.0f,0.0f,0.0f });

	// --- モデルの初期化 ---
	obj3d_ = std::make_unique<Object3d>();
	obj3d_->Initialize("Player/playerBody.obj");

	// --- 各パーツの初期化・設定 ---
	InitArm();

	// --- 各ステータスの初期値設定 ---
	isMove_ = true;
	gameState_ = GameState::kPlaying;

	// --- 各エフェクト・演出の初期設定 ---
	hitEffect_ = std::make_unique<ParticleEmitter>();
	hitEffect_->Initialize("hitEffect", "debug/ringPlane.obj");

	// 被弾用パーティクル
	damageEffect_ = std::make_unique<ParticleEmitter>();
	damageEffect_->Initialize("playerDamage", "debug/ringPlane.obj");

	// 軌跡パーティクルの初期化
	trailEffect_ = std::make_unique<ParticleEmitter>();
	trailEffect_->Initialize("playerTrail", "debug/plane.obj");
	Vector3 initialFootPos = BaseObject::GetWorldPosition();
	initialFootPos.y += kFootOffsetY_;
	lastTrailPosition_ = initialFootPos;
}

void Player::UpdateStartEffect() {
	BaseObject::Update();
	for (const std::unique_ptr<PlayerArm>& arm : arms_) {
		arm->Update();
	}

	if (easeT < 1.0f) {
		easeT += (1.0f / 60.0f) / 1.5f;
		BaseObject::SetScale(Lerp(Vector3{ 0.0f,0.0f,0.0f }, Vector3{ 1.0f,1.0f,1.0f }, easeT));
	}
	else {
		isEnd = true;
	}
}

void Player::Update()
{
	BaseObject::Update();

	// ゲーム状態に応じた更新処理
	switch (gameState_) {
	case GameState::kPlaying:
		// ゲームプレイ中の処理
		if (isAlive_) {
			// 被弾リアクション中の処理
			if (isHitReacting_) {
				UpdateHitReaction();
			}
			// 回避中の処理
			else if (behavior_ == Behavior::kDodge) {
				UpdateDodge();
			}
			else {
				// 通常の移動と攻撃処理
				hitEffect_->SetPosition(BaseObject::GetWorldPosition());
				Move();
				UpdateAttack();
			}
		}

		// アニメーションの再生
		obj3d_->UpdateAnimation(true);

		// 腕の更新
		for (const std::unique_ptr<PlayerArm>& arm : arms_) {
			arm->Update();
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

#ifdef _DEBUG
	hitEffect_->Update(*vp_);
	damageEffect_->Update(*vp_);
	trailEffect_->UpdateOnce(*vp_);
#endif // _DEBUG
}

void Player::UpdateGameOverEffect()
{
	// ゲームオーバー時の演出
	obj3d_->UpdateAnimation(true);

	// 回転しながら縮小
	BaseObject::SetRotation(Vector3(
		BaseObject::GetWorldRotation().x,
		BaseObject::GetWorldRotation().y + 0.5f,
		BaseObject::GetWorldRotation().z));

	if (BaseObject::GetWorldSize().x >= 0.0f) {
		BaseObject::SetScale(Vector3(
			BaseObject::GetWorldSize().x - 0.02f,
			BaseObject::GetWorldSize().y - 0.02f,
			BaseObject::GetWorldSize().z - 0.02f));
	}
	else {
		isAlive_ = false;
	}

	// 腕も更新
	for (const std::unique_ptr<PlayerArm>& arm : arms_) {
		arm->Update();
	}
}

void Player::UpdateGameClearEffect()
{
	// ゲームクリア時はジャンプし続ける演出
	obj3d_->UpdateAnimation(true);

	// ジャンプの周期を計算（40フレームで1サイクル）
	const float kJumpCycle = 40.0f;
	const float kJumpHeight = 2.0f;  // ジャンプの高さ

	// 初回実行時に開始位置を保存
	if (clearEffectTimer_ == 0.0f) {
		clearStartPos_ = BaseObject::GetWorldPosition();
	}

	// 現在のジャンプフェーズを計算（0.0～1.0）
	float jumpPhase = fmod(clearEffectTimer_, kJumpCycle) / kJumpCycle;

	// sin波を使って滑らかなジャンプモーションを作成
	float jumpOffset = sin(jumpPhase * 3.14159265f) * kJumpHeight;

	// 地面の位置にジャンプオフセットを加える
	Vector3 currentPos = clearStartPos_;
	currentPos.y += jumpOffset;

	BaseObject::SetWorldPosition(currentPos);

	// 回転のアニメーション（オプション）
	Vector3 currentRotation = BaseObject::GetWorldRotation();
	currentRotation.y += 0.05f;  // ゆっくり回転
	BaseObject::SetRotation(currentRotation);

	// タイマーを進める
	clearEffectTimer_++;

	// タイマーがオーバーフローしないようにリセット
	if (clearEffectTimer_ >= kJumpCycle * 1000.0f) {
		clearEffectTimer_ = 0.0f;
	}

	// 腕も更新
	for (const std::unique_ptr<PlayerArm>& arm : arms_) {
		arm->Update();
	}
}

void Player::UpdateAttack()
{
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		if (arms_[kLArm] && arms_[kLArm]->CanStartRush()) {
			if (arms_[kRArm]) {
				arms_[kRArm]->StartRush();
			}
			if (arms_[kLArm]) {
				arms_[kLArm]->StartRush();
			}
		}
		else if (arms_[kRArm] && arms_[kRArm]->CanCombo() &&
			arms_[kRArm]->GetLastAttackType() == PlayerArm::AttackType::kRightPunch) {
			if (arms_[kLArm]) {
				arms_[kLArm]->StartAttack(PlayerArm::AttackType::kLeftPunch);
			}
		}
		else {
			bool anyAttacking = false;
			for (const std::unique_ptr<PlayerArm>& arm : arms_) {
				if (arm->GetBehavior() == PlayerArm::Behavior::kAttack ||
					arm->GetBehavior() == PlayerArm::Behavior::kRush) {
					anyAttacking = true;
					break;
				}
			}

			if (!anyAttacking && arms_[kRArm]) {
				arms_[kRArm]->StartAttack(PlayerArm::AttackType::kRightPunch);
			}
		}
	}
}

void Player::StartDodge(const Vector3& direction)
{
	behavior_ = Behavior::kDodge;
	dodgeTimer_ = 0;
	dodgeStartRotation_ = BaseObject::GetTransform().rotation_;

	Vector3 localDirection = direction;
	if (localDirection.Length() > 0.0f) {
		localDirection = localDirection.Normalize();
	}

	float rotY = BaseObject::GetTransform().rotation_.y;
	Matrix4x4 rotMat = MakeRotateYMatrix(rotY);

	dodgeDirection_ = TransformNormal(localDirection, rotMat);

	float worldX = dodgeDirection_.x;
	float worldZ = dodgeDirection_.z;

	float absWorldX = std::abs(worldX);
	float absWorldZ = std::abs(worldZ);

	if (absWorldZ > absWorldX) {
		if (worldZ > 0.0f) {
			dodgeTiltRotation_ = Vector3(kDodgeTiltAngle_, 0.0f, 0.0f);
		}
		else {
			dodgeTiltRotation_ = Vector3(-kDodgeTiltAngle_, 0.0f, 0.0f);
		}
	}
	else {
		if (worldX > 0.0f) {
			dodgeTiltRotation_ = Vector3(0.0f, 0.0f, -kDodgeTiltAngle_);
		}
		else {
			dodgeTiltRotation_ = Vector3(0.0f, 0.0f, kDodgeTiltAngle_);
		}
	}

	if (absWorldX > 0.3f && absWorldZ > 0.3f) {
		float xTilt = worldZ > 0.0f ? kDodgeTiltAngle_ * 0.7f : -kDodgeTiltAngle_ * 0.7f;
		float zTilt = worldX > 0.0f ? -kDodgeTiltAngle_ * 0.7f : kDodgeTiltAngle_ * 0.7f;
		dodgeTiltRotation_ = Vector3(xTilt, 0.0f, zTilt);
	}

	velocity_ = Vector3(0.0f, 0.0f, 0.0f);
}

void Player::UpdateDodge()
{
	dodgeTimer_++;

	float tiltProgress = 0.0f;

	if (dodgeTimer_ < kDodgeTiltInDuration_) {
		float t = static_cast<float>(dodgeTimer_) / kDodgeTiltInDuration_;
		tiltProgress = 1.0f - (1.0f - t) * (1.0f - t);
	}
	else if (dodgeTimer_ >= kDodgeDuration_ - kDodgeTiltOutDuration_) {
		int outTimer = dodgeTimer_ - (kDodgeDuration_ - kDodgeTiltOutDuration_);
		float t = static_cast<float>(outTimer) / kDodgeTiltOutDuration_;
		tiltProgress = 1.0f - (t * t);
	}
	else {
		tiltProgress = 1.0f;
	}

	Vector3 currentTilt = dodgeTiltRotation_ * tiltProgress;

	Vector3 currentRotation;
	currentRotation.x = currentTilt.x;
	currentRotation.y = dodgeStartRotation_.y;
	currentRotation.z = currentTilt.z;

	BaseObject::SetRotation(currentRotation);

	Vector3 currentPos = BaseObject::GetWorldPosition();
	Vector3 moveAmount = dodgeDirection_ * kDodgeSpeed_;
	Vector3 newPos = currentPos + moveAmount;

	if (stageManager_ != nullptr) {
		if (!stageManager_->IsWithinStageBounds(newPos)) {
			newPos = stageManager_->ClampToStageBounds(newPos);
		}
	}

	BaseObject::SetWorldPosition(newPos);

	if (dodgeTimer_ >= kDodgeDuration_) {
		behavior_ = Behavior::kRoot;
		dodgeTimer_ = 0;
		BaseObject::SetRotation(Vector3(0.0f, dodgeStartRotation_.y, 0.0f));
	}
}

void Player::UpdateTrailEffect()
{
	Vector3 currentPos = BaseObject::GetWorldPosition();

	// 足元の位置を計算
	Vector3 footPosition = currentPos;
	footPosition.y += kFootOffsetY_;  // Y座標を足元に調整

	float distanceMoved = (footPosition - lastTrailPosition_).Length();

	// 一定距離移動したらパーティクルを発生
	if (distanceMoved >= trailEmitDistance_) {
		// 移動している場合のみパーティクルを発生
		if (velocity_.Length() > 0.01f) {
			trailEffect_->SetPosition(footPosition);  // 足元の位置を設定
			trailEffect_->SetActive(false);
			isTrailActive_ = true;
		}
		lastTrailPosition_ = footPosition;  // 足元の位置を記録
	}
}

void Player::TakeDamage(const Vector3& hitPosition)
{
	// すでに被弾リアクション中または回避中なら無視
	if (isHitReacting_ || behavior_ == Behavior::kDodge) {
		return;
	}

	isHitReacting_ = true;
	hitReactionTimer_ = 0;
	originalPosition_ = BaseObject::GetWorldPosition();

	// 被弾パーティクルを発生
	damageEffect_->SetPosition(hitPosition);
	damageEffect_->SetActive(false);
}

void Player::UpdateHitReaction()
{
	hitReactionTimer_++;

	// ランダムなシェイクオフセットを生成
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

	// シェイクの強度を時間とともに減衰
	float intensity = kHitShakeIntensity_ * (1.0f - static_cast<float>(hitReactionTimer_) / kHitReactionDuration_);

	hitShakeOffset_.x = dist(gen) * intensity;
	hitShakeOffset_.y = dist(gen) * intensity;
	hitShakeOffset_.z = dist(gen) * intensity;

	// シェイクを適用した位置を設定
	BaseObject::SetWorldPosition(originalPosition_ + hitShakeOffset_);

	// リアクション終了
	if (hitReactionTimer_ >= kHitReactionDuration_) {
		isHitReacting_ = false;
		hitReactionTimer_ = 0;
		hitShakeOffset_ = Vector3(0.0f, 0.0f, 0.0f);
		BaseObject::SetWorldPosition(originalPosition_);
	}
}

void Player::Draw(const ViewProjection& viewProjection)
{
	if (isAlive_) {
		obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection);
	}
}

void Player::DrawAnimation(const ViewProjection& viewProjection)
{
	if (isAlive_) {
		for (const std::unique_ptr<PlayerArm>& arm : arms_) {
			arm->DrawAnimation(viewProjection);
		}
	}
}

void Player::DrawParticle(const ViewProjection& viewProjection)
{
	hitEffect_->Draw(Ring);
	damageEffect_->Draw(Ring);
	trailEffect_->Draw(Normal);
}

void Player::ImGui()
{
	hitEffect_->imgui();
	damageEffect_->imgui();
	trailEffect_->imgui();

	for (size_t i = 0; i < arms_.size(); ++i) {
		if (arms_[i]) {
			arms_[i]->ImGui();
		}
	}

	ImGui::Begin("Player Debug");
	ImGui::Text("Player Rotation Y: %.2f", BaseObject::GetTransform().rotation_.y);
	ImGui::Text("Player Position: (%.2f, %.2f, %.2f)",
		BaseObject::GetWorldPosition().x,
		BaseObject::GetWorldPosition().y,
		BaseObject::GetWorldPosition().z);

	const char* gameStateStr = "Unknown";
	switch (gameState_) {
	case GameState::kPlaying: gameStateStr = "Playing"; break;
	case GameState::kGameOver: gameStateStr = "GameOver"; break;
	case GameState::kGameClear: gameStateStr = "GameClear"; break;
	}
	ImGui::Text("Game State: %s", gameStateStr);

	ImGui::Text("Behavior: %s",
		behavior_ == Behavior::kRoot ? "Root" :
		behavior_ == Behavior::kAttack ? "Attack" :
		behavior_ == Behavior::kDodge ? "Dodge" : "Unknown");

	// 被弾リアクション情報
	ImGui::Separator();
	ImGui::Text("Hit Reaction: %s", isHitReacting_ ? "Active" : "Inactive");
	if (isHitReacting_) {
		ImGui::Text("Reaction Timer: %d / %d", hitReactionTimer_, kHitReactionDuration_);
		ImGui::Text("Shake Offset: (%.3f, %.3f, %.3f)",
			hitShakeOffset_.x, hitShakeOffset_.y, hitShakeOffset_.z);
	}

	if (behavior_ == Behavior::kDodge) {
		ImGui::Text("Dodge Timer: %d / %d", dodgeTimer_, kDodgeDuration_);
		ImGui::Text("Dodge Direction: (%.2f, %.2f, %.2f)",
			dodgeDirection_.x, dodgeDirection_.y, dodgeDirection_.z);
	}

	ImGui::Text("Global Combo Count: %d", globalComboCount_);
	ImGui::Text("Global Combo Timer: %d", globalComboTimer_);

	if (arms_[kRArm]) {
		ImGui::Text("Right Arm State: %s",
			arms_[kRArm]->GetBehavior() == PlayerArm::Behavior::kAttack ? "Attacking" :
			arms_[kRArm]->GetBehavior() == PlayerArm::Behavior::kRush ? "Rush" : "Normal");
	}
	if (arms_[kLArm]) {
		ImGui::Text("Left Arm State: %s",
			arms_[kLArm]->GetBehavior() == PlayerArm::Behavior::kAttack ? "Attacking" :
			arms_[kLArm]->GetBehavior() == PlayerArm::Behavior::kRush ? "Rush" : "Normal");
	}

	ImGui::End();
}

void Player::OnCollision(Collider* other)
{
	uint32_t typeID = other->GetTypeID();

	if (typeID == static_cast<uint32_t>(CollisionTypeIdDef::kEnemy)) {
		Enemy* enemy = static_cast<Enemy*>(other);

		if (GetSerialNumber() > enemy->GetSerialNumber()) {
			return;
		}

		// 回避中は当たり判定を無視
		if (behavior_ == Behavior::kDodge) {
			return;
		}

		// 既に被弾リアクション中なら無視
		if (isHitReacting_) {
			return;
		}

		// 被弾処理を呼び出す
		Vector3 hitPos = (GetCenterPosition() + enemy->GetCenterPosition()) * 0.5f;
		TakeDamage(hitPos);

		// ステージ境界チェック
		Vector3 playerPos = BaseObject::GetWorldPosition();
		if (stageManager_ != nullptr && !stageManager_->IsWithinStageBounds(playerPos)) {
			playerPos = stageManager_->ClampToStageBounds(playerPos);
			BaseObject::SetWorldPosition(playerPos);
		}
	}

	transform_.UpdateMatrix();
}

void Player::InitArm()
{
	for (std::unique_ptr<PlayerArm>& arm : arms_) {
		arm = std::make_unique<PlayerArm>();
		arm->SetPlayer(this);
	}

	arms_[kRArm]->Init("player/Arm/playerArm.gltf");
	arms_[kRArm]->SetID(serialNumber_);
	arms_[kRArm]->SetIsRightArm(true);
	arms_[kRArm]->SetTranslation(Vector3(-1.7f, 0.0f, 1.3f));
	arms_[kRArm]->SetScale(Vector3(0.8f, 0.8f, 0.8f));

	arms_[kLArm]->Init("player/Arm/playerArm.gltf");
	arms_[kLArm]->SetID(serialNumber_);
	arms_[kLArm]->SetIsRightArm(false);
	arms_[kLArm]->SetTranslation(Vector3(1.7f, 0.0f, 1.3f));
	arms_[kLArm]->SetScale(Vector3(0.8f, 0.8f, 0.8f));
}

void Player::Move()
{
	Vector3 move = { 0.0f, 0.0f, 0.0f };

	if (Input::GetInstance()->PushKey(DIK_D)) {
		move.x += kAcceleration_;
	}
	if (Input::GetInstance()->PushKey(DIK_A)) {
		move.x -= kAcceleration_;
	}
	if (Input::GetInstance()->PushKey(DIK_W)) {
		move.z += kAcceleration_;
	}
	if (Input::GetInstance()->PushKey(DIK_S)) {
		move.z -= kAcceleration_;
	}

	float currentRotationY = BaseObject::GetTransform().rotation_.y;
	if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
		currentRotationY += kRotateAcceleration_;
	}
	if (Input::GetInstance()->PushKey(DIK_LEFT)) {
		currentRotationY -= kRotateAcceleration_;
	}

	Vector3 currentRotation = BaseObject::GetTransform().rotation_;
	currentRotation.y = currentRotationY;
	BaseObject::SetRotation(currentRotation);

	if (Input::GetInstance()->TriggerKey(DIK_LSHIFT) || Input::GetInstance()->TriggerKey(DIK_RSHIFT)) {
		if (move.Length() != 0.0f) {
			Vector3 localMove = move.Normalize();
			StartDodge(localMove);
			return;
		}
	}

	if (move.Length() != 0.0f) {
		move.Normalize();

		float rotY = currentRotationY;
		Matrix4x4 rotMat = MakeRotateYMatrix(rotY);

		move = TransformNormal(move, rotMat);

		velocity_ += move * kAcceleration_;
	}
	else {
		velocity_ = 0.0f;
	}

	if (velocity_.Length() > kMaxSpeed_) {
		velocity_ = velocity_.Normalize() * kMaxSpeed_;
	}

	Vector3 currentPos = BaseObject::GetWorldPosition();
	Vector3 newPos = currentPos + velocity_;

	if (stageManager_ != nullptr) {
		if (!stageManager_->IsWithinStageBounds(newPos)) {
			newPos = stageManager_->ClampToStageBounds(newPos);
			velocity_ *= 0.5f;
		}
	}

	BaseObject::SetWorldPosition(newPos);

	UpdateTrailEffect();
}