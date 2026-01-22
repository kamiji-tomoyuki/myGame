#include "PlayerArm.h"
#include <CollisionTypeIdDef.h>
#include "Input.h"
#include "myMath.h"
#include "Player.h"
#include <Enemy.h>

PlayerArm::PlayerArm()
{
	serialNumber_ = nextSerialNumber_;
	nextSerialNumber_++;
}

void PlayerArm::Init(std::string filePath)
{
	BaseObject::Init();
	Collider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPArm));

	// --- モデルの初期化 ---
	obj3d_ = std::make_unique<Object3d>();
	obj3d_->Initialize(filePath);

	// 初期位置を保存
	originalPosition_ = transform_.translation_;
	targetPosition_ = originalPosition_;

	// ダメージ設定
	attackDamage_ = 50;      // 通常攻撃のダメージ
	rushAttackDamage_ = 20;  // ラッシュ攻撃1ヒットあたりのダメージ
}

void PlayerArm::Update()
{
	BaseObject::Update();

	// 攻撃処理の更新
	UpdateAttack();

	// ラッシュ処理の更新
	UpdateRush();

	// コンボタイマーの更新
	UpdateComboTime();

	obj3d_->UpdateAnimation(true);
}

void PlayerArm::UpdateComboTime()
{
	if (comboTimer_ > 0) {
		comboTimer_--;
		if (comboTimer_ == 0) {
			comboCount_ = 0;
		}
	}
}

void PlayerArm::UpdateAttack()
{
	if (behavior_ != Behavior::kAttack) {
		return;
	}

	attackTimer_++;

	attackProgress_ = static_cast<float>(attackTimer_) / static_cast<float>(kAttackDuration);

	if (attackProgress_ >= 1.0f) {
		attackProgress_ = 1.0f;
	}

	float easedProgress = 1.0f - (1.0f - attackProgress_) * (1.0f - attackProgress_);
	if (attackProgress_ > 0.5f) {
		easedProgress = 1.0f - (attackProgress_ - 0.5f) * 2.0f;
	}

	Vector3 currentPos = {
		originalPosition_.x + (targetPosition_.x - originalPosition_.x) * easedProgress,
		originalPosition_.y + (targetPosition_.y - originalPosition_.y) * easedProgress,
		originalPosition_.z + (targetPosition_.z - originalPosition_.z) * easedProgress
	};
	transform_.translation_ = currentPos;

	if (attackTimer_ >= kAttackDuration) {
		behavior_ = Behavior::kNormal;
		isAttack_ = false;
		attackTimer_ = 0;
		attackProgress_ = 0.0f;
		transform_.translation_ = originalPosition_;

		lastAttackType_ = currentAttackType_;
		currentAttackType_ = AttackType::kNone;
	}
}

void PlayerArm::UpdateRush()
{
	if (behavior_ != Behavior::kRush) {
		return;
	}

	rushTimer_++;

	if (rushTimer_ % kRushInterval == 0) {
		rushAttackActive_ = true;
		rushAttackTimer_ = 0;
		rushCount_++;

		float randomOffset = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f;

		Vector3 forward = { 0.0f, 0.0f, 1.0f };
		Vector3 attackOffset = {
			forward.x * kRushDistance + randomOffset,
			forward.y * kRushDistance,
			forward.z * kRushDistance
		};

		targetPosition_ = {
			originalPosition_.x + attackOffset.x,
			originalPosition_.y + attackOffset.y,
			originalPosition_.z + attackOffset.z
		};

		attackDirection_ = attackOffset;
		float attackDirLength = sqrt(attackDirection_.x * attackDirection_.x +
			attackDirection_.y * attackDirection_.y +
			attackDirection_.z * attackDirection_.z);
		if (attackDirLength > 0.0f) {
			attackDirection_.x /= attackDirLength;
			attackDirection_.y /= attackDirLength;
			attackDirection_.z /= attackDirLength;
		}
	}

	if (rushAttackActive_) {
		rushAttackTimer_++;

		float rushProgress = static_cast<float>(rushAttackTimer_) / static_cast<float>(kRushAttackDuration);
		if (rushProgress >= 1.0f) {
			rushProgress = 1.0f;
		}

		float easedProgress = 1.0f - (1.0f - rushProgress) * (1.0f - rushProgress);
		if (rushProgress > 0.6f) {
			easedProgress = 1.0f - (rushProgress - 0.6f) * 2.5f;
		}

		Vector3 currentPos = {
			originalPosition_.x + (targetPosition_.x - originalPosition_.x) * easedProgress,
			originalPosition_.y + (targetPosition_.y - originalPosition_.y) * easedProgress,
			originalPosition_.z + (targetPosition_.z - originalPosition_.z) * easedProgress
		};
		transform_.translation_ = currentPos;

		if (rushAttackTimer_ >= kRushAttackDuration) {
			rushAttackActive_ = false;
			rushAttackTimer_ = 0;
			transform_.translation_ = originalPosition_;
		}
	}

	if (rushTimer_ >= kRushDuration) {
		behavior_ = Behavior::kNormal;
		isRush_ = false;
		rushTimer_ = 0;
		rushAttackTimer_ = 0;
		rushCount_ = 0;
		rushAttackActive_ = false;
		transform_.translation_ = originalPosition_;

		comboCount_ = 0;
		comboTimer_ = 0;
		currentAttackType_ = AttackType::kNone;
		lastAttackType_ = AttackType::kNone;
	}
}

void PlayerArm::StartAttack(AttackType attackType)
{
	if (behavior_ == Behavior::kAttack || behavior_ == Behavior::kRush) {
		return;
	}

	behavior_ = Behavior::kAttack;
	isAttack_ = true;
	currentAttackType_ = attackType;
	attackTimer_ = 0;
	attackProgress_ = 0.0f;

	if (attackType == AttackType::kRightPunch) {
		comboTimer_ = 60;
		comboCount_ = 1;
	}
	else if (attackType == AttackType::kLeftPunch) {
		comboTimer_ = 90;
		comboCount_ = 2;
	}

	Vector3 forward = { 0.0f, 0.0f, 1.0f };
	Vector3 right = { 1.0f, 0.0f, 0.0f };

	float forwardLength = sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
	if (forwardLength > 0.0f) {
		forward.x /= forwardLength;
		forward.y /= forwardLength;
		forward.z /= forwardLength;
	}

	float rightLength = sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
	if (rightLength > 0.0f) {
		right.x /= rightLength;
		right.y /= rightLength;
		right.z /= rightLength;
	}

	Vector3 attackOffset = {
		forward.x * kAttackDistance,
		forward.y * kAttackDistance,
		forward.z * kAttackDistance
	};

	switch (attackType) {
	case AttackType::kRightPunch:
		attackOffset.x += 0.7f;
		attackOffset.y += right.y * kRightPunchOffset;
		attackOffset.z += right.z * kRightPunchOffset;
		break;
	case AttackType::kLeftPunch:
		attackOffset.x += -0.7f;
		attackOffset.y += right.y * kLeftPunchOffset;
		attackOffset.z += right.z * kLeftPunchOffset;
		break;
	}

	originalPosition_ = transform_.translation_;
	targetPosition_ = {
		originalPosition_.x + attackOffset.x,
		originalPosition_.y + attackOffset.y,
		originalPosition_.z + attackOffset.z
	};

	attackDirection_ = attackOffset;
	float attackDirLength = sqrt(attackDirection_.x * attackDirection_.x +
		attackDirection_.y * attackDirection_.y +
		attackDirection_.z * attackDirection_.z);
	if (attackDirLength > 0.0f) {
		attackDirection_.x /= attackDirLength;
		attackDirection_.y /= attackDirLength;
		attackDirection_.z /= attackDirLength;
	}
}

void PlayerArm::StartRush()
{
	if (behavior_ == Behavior::kAttack || behavior_ == Behavior::kRush) {
		return;
	}

	behavior_ = Behavior::kRush;
	isRush_ = true;
	currentAttackType_ = AttackType::kRush;
	rushTimer_ = 0;
	rushAttackTimer_ = 0;
	rushCount_ = 0;
	rushAttackActive_ = false;

	originalPosition_ = transform_.translation_;
}

void PlayerArm::ProcessAttack()
{
	if (!isAttack_ && !isRush_) {
		return;
	}

	if (isAttack_) {
		if (attackProgress_ >= 0.5f && attackProgress_ < 0.6f) {
			// 攻撃判定のタイミング
		}
	}

	if (isRush_ && rushAttackActive_) {
		if (rushAttackTimer_ >= 2 && rushAttackTimer_ <= 6) {
			// ラッシュ攻撃の当たり判定タイミング
		}
	}
}

bool PlayerArm::CanCombo() const
{
	return (comboTimer_ > 0 && behavior_ == Behavior::kNormal);
}

bool PlayerArm::CanStartRush() const
{
	return (behavior_ == Behavior::kNormal &&
		lastAttackType_ == AttackType::kLeftPunch &&
		comboTimer_ > 0);
}

void PlayerArm::Draw(const ViewProjection& viewProjection)
{
}

void PlayerArm::DrawAnimation(const ViewProjection& viewProjection)
{
	obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection);
}

void PlayerArm::DrawParticle(const ViewProjection& viewProjection)
{
}

void PlayerArm::ImGui()
{
	ImGui::Begin("PlayerArm Debug");

	const char* behaviorNames[] = { "Normal", "Attack", "Skill", "Rush" };
	ImGui::Text("Behavior: %s", behaviorNames[static_cast<int>(behavior_)]);

	const char* attackTypeNames[] = { "None", "RightPunch", "LeftPunch", "Rush" };
	ImGui::Text("Current Attack: %s", attackTypeNames[static_cast<int>(currentAttackType_)]);
	ImGui::Text("Last Attack: %s", attackTypeNames[static_cast<int>(lastAttackType_)]);

	ImGui::Text("Attack Timer: %d / %d", attackTimer_, kAttackDuration);
	ImGui::Text("Combo Timer: %d", comboTimer_);
	ImGui::Text("Combo Count: %d", comboCount_);
	ImGui::Text("Attack Progress: %.2f", attackProgress_);

	ImGui::Text("Rush Timer: %d / %d", rushTimer_, kRushDuration);
	ImGui::Text("Rush Count: %d", rushCount_);
	ImGui::Text("Rush Attack Active: %s", rushAttackActive_ ? "Yes" : "No");

	ImGui::Text("Can Combo: %s", CanCombo() ? "Yes" : "No");
	ImGui::Text("Can Start Rush: %s", CanStartRush() ? "Yes" : "No");
	ImGui::Text("Is Attack: %s", isAttack_ ? "Yes" : "No");
	ImGui::Text("Is Rush: %s", isRush_ ? "Yes" : "No");

	ImGui::Text("Original Pos: (%.2f, %.2f, %.2f)",
		originalPosition_.x, originalPosition_.y, originalPosition_.z);
	ImGui::Text("Target Pos: (%.2f, %.2f, %.2f)",
		targetPosition_.x, targetPosition_.y, targetPosition_.z);
	ImGui::Text("Attack Direction: (%.2f, %.2f, %.2f)",
		attackDirection_.x, attackDirection_.y, attackDirection_.z);

	ImGui::Separator();
	ImGui::Text("Damage Settings:");
	ImGui::Text("Attack Damage: %d", attackDamage_);
	ImGui::Text("Rush Attack Damage: %d", rushAttackDamage_);

	ImGui::End();
}

void PlayerArm::OnCollision(Collider* other)
{
	// 攻撃中またはラッシュ中でない場合は処理しない
	if (!isAttack_ && !isRush_) {
		return;
	}

	uint32_t typeID = other->GetTypeID();

	// 敵との当たり判定
	if (typeID == static_cast<uint32_t>(CollisionTypeIdDef::kEnemy)) {
		Enemy* enemy = static_cast<Enemy*>(other);

		// ヒット位置を計算（腕と敵の中間地点）
		Vector3 hitPosition = (GetCenterPosition() + enemy->GetCenterPosition()) * 0.5f;

		// ラッシュ攻撃中
		if (isRush_ && rushAttackActive_) {
			// ラッシュ攻撃の有効フレーム内でのみダメージを与える
			if (rushAttackTimer_ >= 2 && rushAttackTimer_ <= 6) {
				// 連続ヒットを防ぐため、少し待機時間を設ける
				static int lastRushHitFrame = -999;
				int currentFrame = rushTimer_;

				// 前回のヒットから一定フレーム経過していれば新しいダメージを与える
				if (currentFrame - lastRushHitFrame >= 3) {
					enemy->TakeDamage(rushAttackDamage_/*, hitPosition*/);
					lastRushHitFrame = currentFrame;
				}
			}
		}
		// 通常攻撃中
		else if (isAttack_) {
			// 攻撃の最大前進時のみダメージを与える
			if (attackProgress_ >= 0.4f && attackProgress_ <= 0.6f) {
				// 1回の攻撃で1回だけダメージを与えるためのフラグ
				static bool hasHitThisAttack = false;
				static int lastAttackId = -1;
				int currentAttackId = attackTimer_ + (isRightArm_ ? 10000 : 20000);

				// 新しい攻撃の場合はフラグをリセット
				if (currentAttackId != lastAttackId) {
					hasHitThisAttack = false;
					lastAttackId = currentAttackId;
				}

				// まだこの攻撃でヒットしていなければダメージを与える
				if (!hasHitThisAttack) {
					enemy->TakeDamage(attackDamage_/*, hitPosition*/);
					hasHitThisAttack = true;
				}
			}
		}
	}
}

void PlayerArm::SetPlayer(Player* player)
{
	player_ = player;
	BaseObject::transform_.parent_ = &player->GetWorldTransform();
}