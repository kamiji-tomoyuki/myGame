#include "Player.h"
#include "Input.h"

#include "FollowCamera.h"
#include "CollisionTypeIdDef.h"

#include "myMath.h"
#include <Enemy.h>
#include <EnemyAttackManager.h>
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
	BaseObject::SetScale({ 0.0f, 0.0f, 0.0f });

	// --- モデルの初期化 ---
	obj3d_ = std::make_unique<Object3d>();
	obj3d_->Initialize("Player/playerBody.obj");

	// --- HPバー背景の初期化 ---
	hpBarBg_ = std::make_unique<Sprite>();
	hpBarBg_->Initialize("white1x1.png", {
		40.0f - kHpBarBgPadding_,
		150.0f - kHpBarBgPadding_
		});
	hpBarBg_->SetColor({ 0.2f, 0.2f, 0.2f });  // ダークグレー
	hpBarBg_->SetSize({
		kHpBarFullWidth_ + kHpBarBgPadding_ * 2.0f,
		kHpBarHeight_ + kHpBarBgPadding_ * 2.0f
		});

	// --- HPバーの初期化 ---
	hpBar_ = std::make_unique<Sprite>();
	hpBar_->Initialize("white1x1.png", { 40.0f, 150.0f });
	hpBar_->SetColor(hpColor_);
	hpBar_->SetSize({ kHpBarFullWidth_, kHpBarHeight_ });

	// --- 各パーツの初期化・設定 ---
	InitArm();

	// --- 各ステータスの初期値設定 ---
	isMove_ = true;
	gameState_ = GameState::kPlaying;

	// --- 各エフェクト・演出の初期設定 ---
	hitEffect_ = std::make_unique<ParticleEmitter>();
	hitEffect_->Initialize("hitEffect", "debug/ringPlane.obj");

	damageEffect_ = std::make_unique<ParticleEmitter>();
	damageEffect_->Initialize("playerDamage", "debug/ringPlane.obj");

	trailEffect_ = std::make_unique<ParticleEmitter>();
	trailEffect_->Initialize("playerTrail", "debug/plane.obj");
	{
		Vector3 initialFootPos = BaseObject::GetWorldPosition();
		initialFootPos.y += kFootOffsetY_;
		lastTrailPosition_ = initialFootPos;
	}

	// =============================================================
	// サブクラスの生成
	// =============================================================
	startEffect_ = std::make_unique<PlayerStartEffect>(this);
	gameOverEffect_ = std::make_unique<PlayerGameOverEffect>(this);
	gameClearEffect_ = std::make_unique<PlayerGameClearEffect>(this);
	attack_ = std::make_unique<PlayerAttack>(this, arms_);
	dodge_ = std::make_unique<PlayerDodge>(this, stageManager_);
}

// =============================================================
// 開始演出の更新（外部から呼ぶ）
// =============================================================
void Player::UpdateStartEffect()
{
	BaseObject::Update();
	startEffect_->Update();
}

// =============================================================
// メインUpdate
// =============================================================
void Player::Update()
{
	BaseObject::Update();

	switch (gameState_) {
	case GameState::kPlaying:
		if (isAlive_) {

			if (contactDamageCooldown_ > 0) {
				contactDamageCooldown_--;
			}

			if (isHitReacting_) {
				UpdateHitReaction();
			}

			if (isHitReacting_) {
				UpdateHitReaction();
			}
			else if (dodge_->IsDodging()) {
				// behavior_ も同期
				behavior_ = Behavior::kDodge;
				dodge_->Update();
			}
			else {
				behavior_ = Behavior::kRoot;

				hitEffect_->SetPosition(BaseObject::GetWorldPosition());
				Move();
				attack_->Update();     // 攻撃
				UpdateLockOn();
			}
		}

		obj3d_->UpdateAnimation(true);
		UpdateArms();  // ★ここで腕のUpdate()が呼ばれる



		break;

	case GameState::kGameOver:
		gameOverEffect_->Update();
		isAlive_ = gameOverEffect_->IsAlive();
		break;

	case GameState::kGameClear:
		gameClearEffect_->Update();
		break;
	}

	// HPバー更新
	float hpRatio = static_cast<float>(HP_) / static_cast<float>(kMaxHP_);
	float currentWidth = kHpBarFullWidth_ * hpRatio;
	hpBar_->SetSize({ currentWidth, kHpBarHeight_ });


	trailEffect_->UpdateOnce(*vp_);
	Collider::UpdateWorldTransform();

#ifdef _DEBUG
	hitEffect_->Update(*vp_);
	damageEffect_->Update(*vp_);
#endif // _DEBUG
}

// =============================================================
// サブクラス用公開メソッド
// =============================================================
void Player::UpdateArms()
{
	for (const auto& arm : arms_) {
		if (arm) {
			arm->Update();
		}
	}
}

void Player::UpdateModelAnimation()
{
	obj3d_->UpdateAnimation(true);
}

// =============================================================
// UI判定・攻撃実行中判定（PlayerAttack に委譲）
// =============================================================
bool Player::CanRightPunch()      const { return attack_->CanRightPunch(); }
bool Player::CanLeftPunch()       const { return attack_->CanLeftPunch(); }
bool Player::CanRush()            const { return attack_->CanRush(); }
bool Player::IsRightPunchActive() const { return attack_->IsRightPunchActive(); }
bool Player::IsLeftPunchActive()  const { return attack_->IsLeftPunchActive(); }
bool Player::IsRushActive()       const { return attack_->IsRushActive(); }

// =============================================================
// ロックオン
// =============================================================
void Player::UpdateLockOn()
{
	if (Input::GetInstance()->TriggerKey(DIK_R)) {
		isLockOn_ = !isLockOn_;
	}

	if (isLockOn_ && enemy_ != nullptr) {
		Vector3 playerPos = GetCenterPosition();
		Vector3 enemyPos = enemy_->GetCenterPosition();

		Vector3 direction = enemyPos - playerPos;
		Vector3 normalizedDirection = direction.Normalize();

		float targetRotationY = std::atan2(normalizedDirection.x, normalizedDirection.z);
		Vector3 currentRotation = GetCenterRotation();
		float   currentRotationY = currentRotation.y;

		float angleDiff = targetRotationY - currentRotationY;

		const float PI = 3.14159265359f;
		while (angleDiff > PI) { angleDiff -= 2.0f * PI; }
		while (angleDiff < -PI) { angleDiff += 2.0f * PI; }

		float lerpFactor = 0.2f;
		float newRotationY = currentRotationY + angleDiff * lerpFactor;

		BaseObject::SetRotation(Vector3(currentRotation.x, newRotationY, currentRotation.z));
	}
}

// =============================================================
// 被弾処理
// =============================================================
void Player::TakeDamage(const Vector3& hitPosition)
{
	if (isHitReacting_ || dodge_->IsDodging()) {
		return;
	}

	isHitReacting_ = true;
	hitReactionTimer_ = 0;
	originalPosition_ = BaseObject::GetWorldPosition();

	damageEffect_->SetPosition(hitPosition);
	damageEffect_->SetActive(false);
}

void Player::UpdateHitReaction()
{
	hitReactionTimer_++;

	static std::random_device                    rd;
	static std::mt19937                          gen(rd());
	static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

	float intensity = kHitShakeIntensity_ *
		(1.0f - static_cast<float>(hitReactionTimer_) / kHitReactionDuration_);

	hitShakeOffset_.x = dist(gen) * intensity;
	hitShakeOffset_.y = dist(gen) * intensity;
	hitShakeOffset_.z = dist(gen) * intensity;

	BaseObject::SetWorldPosition(originalPosition_ + hitShakeOffset_);

	if (hitReactionTimer_ >= kHitReactionDuration_) {
		isHitReacting_ = false;
		hitReactionTimer_ = 0;
		hitShakeOffset_ = Vector3(0.0f, 0.0f, 0.0f);
		BaseObject::SetWorldPosition(originalPosition_);
	}
}

// =============================================================
// 外部ダメージ受け付け（遠距離攻撃など）
// =============================================================
void Player::ApplyDamage(uint32_t damage, const Vector3& hitPosition)
{
	// 回避中・リアクション中なら無視
	if (dodge_->IsDodging() || isHitReacting_) {
		return;
	}

	// HP減少
	if (HP_ > damage) { HP_ -= damage; }
	else {
		HP_ = 0;
		gameState_ = GameState::kGameOver;
	}

	// 被弾リアクション（シェイク・エフェクト）を開始
	TakeDamage(hitPosition);
}

// =============================================================
// 軌跡パーティクル
// =============================================================
void Player::UpdateTrailEffect()
{
	Vector3 currentPos = BaseObject::GetWorldPosition();
	Vector3 footPosition = currentPos;
	footPosition.y += kFootOffsetY_;

	float distanceMoved = (footPosition - lastTrailPosition_).Length();

	if (distanceMoved >= trailEmitDistance_) {
		if (velocity_.Length() > 0.01f) {
			trailEffect_->SetPosition(footPosition);
			trailEffect_->SetActive(false);
			isTrailActive_ = true;
		}
		lastTrailPosition_ = footPosition;
	}
}

// =============================================================
// 移動
// =============================================================
void Player::Move()
{
	Vector3 move = { 0.0f, 0.0f, 0.0f };

	if (Input::GetInstance()->PushKey(DIK_D)) { move.x += kAcceleration_; }
	if (Input::GetInstance()->PushKey(DIK_A)) { move.x -= kAcceleration_; }
	if (Input::GetInstance()->PushKey(DIK_W)) { move.z += kAcceleration_; }
	if (Input::GetInstance()->PushKey(DIK_S)) { move.z -= kAcceleration_; }

	float currentRotationY = BaseObject::GetTransform().rotation_.y;
	if (Input::GetInstance()->PushKey(DIK_RIGHT)) { currentRotationY += kRotateAcceleration_; }
	if (Input::GetInstance()->PushKey(DIK_LEFT)) { currentRotationY -= kRotateAcceleration_; }

	Vector3 currentRotation = BaseObject::GetTransform().rotation_;
	currentRotation.y = currentRotationY;
	BaseObject::SetRotation(currentRotation);

	// --- Shift + 移動 → 回避開始 ---
	if (Input::GetInstance()->TriggerKey(DIK_LSHIFT) ||
		Input::GetInstance()->TriggerKey(DIK_RSHIFT)) {
		if (move.Length() != 0.0f) {
			Vector3 localMove = move.Normalize();
			dodge_->Start(localMove);   // PlayerDodge に委譲
			behavior_ = Behavior::kDodge;
			return;
		}
	}

	// --- 通常移動 ---
	if (move.Length() != 0.0f) {
		move.Normalize();

		Matrix4x4 rotMat = MakeRotateYMatrix(currentRotationY);
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

// =============================================================
// 描画
// =============================================================
void Player::Draw(const ViewProjection& viewProjection)
{
	if (isAlive_) {
		obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection);
	}
}

void Player::DrawAnimation(const ViewProjection& viewProjection)
{
	if (isAlive_) {
		// ★腕の描画
		for (const auto& arm : arms_) {
			if (arm) {
				arm->DrawAnimation(viewProjection);
			}
		}
	}
}

void Player::DrawSprite(const ViewProjection& viewProjection)
{
	hpBarBg_->Draw();   // ← 背景を先に（奥側）
	hpBar_->Draw();     // ← HPバーを後に（手前側）
}

void Player::DrawParticle(const ViewProjection& viewProjection)
{
	hitEffect_->Draw(Ring);
	damageEffect_->Draw(Ring);
	trailEffect_->Draw(Normal);
}

// =============================================================
// ImGui
// =============================================================
void Player::ImGui()
{
	hitEffect_->imgui();
	damageEffect_->imgui();
	trailEffect_->imgui();

	// ★両腕のImGuiを表示
	for (size_t i = 0; i < arms_.size(); ++i) {
		if (arms_[i]) {
			arms_[i]->ImGui();
		}
	}

	ImGui::Begin("Player Debug");

	float hpRatio = static_cast<float>(HP_) / static_cast<float>(kMaxHP_);
	ImVec4 hpColor;
	if (hpRatio > 0.5f) { hpColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); }
	else if (hpRatio > 0.25f) { hpColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); }
	else { hpColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); }
	ImGui::TextColored(hpColor, "HP: %d / %d (%.1f%%)", HP_, kMaxHP_, hpRatio * 100.0f);
	ImGui::ProgressBar(hpRatio, ImVec2(0.0f, 0.0f));

	ImGui::Separator();
	ImGui::Text("Player Rotation Y: %.2f", BaseObject::GetTransform().rotation_.y);
	ImGui::Text("Player Position: (%.2f, %.2f, %.2f)",
		BaseObject::GetWorldPosition().x,
		BaseObject::GetWorldPosition().y,
		BaseObject::GetWorldPosition().z);

	const char* gameStateStr = "Unknown";
	switch (gameState_) {
	case GameState::kPlaying:   gameStateStr = "Playing";   break;
	case GameState::kGameOver:  gameStateStr = "GameOver";  break;
	case GameState::kGameClear: gameStateStr = "GameClear"; break;
	}
	ImGui::Text("Game State: %s", gameStateStr);

	ImGui::Text("Behavior: %s",
		behavior_ == Behavior::kRoot ? "Root" :
		behavior_ == Behavior::kAttack ? "Attack" :
		behavior_ == Behavior::kDodge ? "Dodge" : "Unknown");

	ImGui::Separator();
	ImGui::Text("Lock-On: %s", isLockOn_ ? "ON" : "OFF");

	ImGui::Separator();
	ImGui::Text("Hit Reaction: %s", isHitReacting_ ? "Active" : "Inactive");
	if (isHitReacting_) {
		ImGui::Text("Reaction Timer: %d / %d", hitReactionTimer_, kHitReactionDuration_);
		ImGui::Text("Shake Offset: (%.3f, %.3f, %.3f)",
			hitShakeOffset_.x, hitShakeOffset_.y, hitShakeOffset_.z);
	}

	if (dodge_->IsDodging()) {
		ImGui::Text("Dodge: Active");
	}

	ImGui::Text("Global Combo Count: %d", globalComboCount_);
	ImGui::Text("Global Combo Timer: %d", globalComboTimer_);

	if (arms_[kRArm]) {
		ImGui::Text("Right Arm State: %s",
			arms_[kRArm]->GetBehavior() == PlayerArm::Behavior::kAttack ? "Attacking" :
			arms_[kRArm]->GetBehavior() == PlayerArm::Behavior::kRush ? "Rush" : "Normal");
		ImGui::Text("Right Arm Collision: %s", arms_[kRArm]->IsCollisionEnabled() ? "Enabled" : "Disabled");
	}
	if (arms_[kLArm]) {
		ImGui::Text("Left Arm State: %s",
			arms_[kLArm]->GetBehavior() == PlayerArm::Behavior::kAttack ? "Attacking" :
			arms_[kLArm]->GetBehavior() == PlayerArm::Behavior::kRush ? "Rush" : "Normal");
		ImGui::Text("Left Arm Collision: %s", arms_[kLArm]->IsCollisionEnabled() ? "Enabled" : "Disabled");
	}

	ImGui::End();
}

// =============================================================
// OnCollision
// =============================================================
void Player::OnCollision(Collider* other)
{
	uint32_t typeID = other->GetTypeID();

	if (typeID == static_cast<uint32_t>(CollisionTypeIdDef::kEnemy)) {
		Enemy* enemy = static_cast<Enemy*>(other);

		if (dodge_->IsDodging()) { return; }
		if (isHitReacting_) { return; }

		if (enemy->IsAttacking()) {
			uint32_t attackDamage = 100;
			if (HP_ > attackDamage) { HP_ -= attackDamage; }
			else {
				HP_ = 0;
				gameState_ = GameState::kGameOver;
			}

			Vector3 hitPos = (GetCenterPosition() + enemy->GetCenterPosition()) * 0.5f;
			TakeDamage(hitPos);

			Vector3 knockbackDirection = (GetCenterPosition() - enemy->GetCenterPosition()).Normalize();
			velocity_ += knockbackDirection * 0.5f;
		}
		else {
			// クールダウン中なら接触ダメージを与えない
			if (contactDamageCooldown_ > 0) { return; }

			uint32_t contactDamage = 10;
			if (HP_ > contactDamage) { HP_ -= contactDamage; }
			else {
				HP_ = 0;
				gameState_ = GameState::kGameOver;
			}

			Vector3 hitPos = (GetCenterPosition() + enemy->GetCenterPosition()) * 0.5f;
			TakeDamage(hitPos);
		}

		Vector3 playerPos = BaseObject::GetWorldPosition();
		if (stageManager_ != nullptr && !stageManager_->IsWithinStageBounds(playerPos)) {
			playerPos = stageManager_->ClampToStageBounds(playerPos);
			BaseObject::SetWorldPosition(playerPos);
		}
	}

	transform_.UpdateMatrix();
}

// =============================================================
// InitArm 
// =============================================================
void Player::InitArm()
{
	// ★右腕の初期化
	arms_[kRArm] = std::make_unique<PlayerArm>();
	arms_[kRArm]->Init("player/Arm/playerArm.gltf");
	arms_[kRArm]->SetPlayer(this);
	arms_[kRArm]->SetID(serialNumber_);
	arms_[kRArm]->SetColliderID(CollisionTypeIdDef::kPRArm);
	arms_[kRArm]->SetTranslation(Vector3(-1.7f, 0.0f, 1.3f));
	arms_[kRArm]->SetScale(Vector3(0.8f, 0.8f, 0.8f));

	// ★左腕の初期化
	arms_[kLArm] = std::make_unique<PlayerArm>();
	arms_[kLArm]->Init("player/Arm/playerArm.gltf");
	arms_[kLArm]->SetPlayer(this);
	arms_[kLArm]->SetID(serialNumber_);
	arms_[kLArm]->SetColliderID(CollisionTypeIdDef::kPLArm);
	arms_[kLArm]->SetTranslation(Vector3(1.7f, 0.0f, 1.3f));
	arms_[kLArm]->SetScale(Vector3(0.8f, 0.8f, 0.8f));
}