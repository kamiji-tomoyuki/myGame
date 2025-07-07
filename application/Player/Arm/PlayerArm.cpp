#include "PlayerArm.h"
#include <CollisionTypeIdDef.h>
#include "Input.h"
#include "myMath.h"
#include "Player.h"

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
}

void PlayerArm::Update()
{
	BaseObject::Update();

	// 攻撃処理の更新
	UpdateAttack();

	// コンボタイマーの更新
	UpdateComboTime();

	obj3d_->AnimationUpdate(true);
}

void PlayerArm::UpdateComboTime()
{
	if (comboTimer_ > 0) {
		comboTimer_--;
	}
}

void PlayerArm::UpdateAttack()
{
	if (behavior_ != Behavior::kAttack) {
		return;
	}

	// 攻撃タイマーの更新
	attackTimer_++;

	// 攻撃の進行度を計算（0.0f〜1.0f）
	attackProgress_ = static_cast<float>(attackTimer_) / static_cast<float>(kAttackDuration);

	if (attackProgress_ >= 1.0f) {
		attackProgress_ = 1.0f;
	}

	// 攻撃アニメーション（イージング）
	float easedProgress = 1.0f - (1.0f - attackProgress_) * (1.0f - attackProgress_); // イーズアウト
	if (attackProgress_ > 0.5f) {
		// 後半は元の位置に戻る
		easedProgress = 1.0f - (attackProgress_ - 0.5f) * 2.0f;
	}

	// 現在の位置を計算（線形補間）
	Vector3 currentPos = {
		originalPosition_.x + (targetPosition_.x - originalPosition_.x) * easedProgress,
		originalPosition_.y + (targetPosition_.y - originalPosition_.y) * easedProgress,
		originalPosition_.z + (targetPosition_.z - originalPosition_.z) * easedProgress
	};
	transform_.translation_ = currentPos;

	// 攻撃終了判定
	if (attackTimer_ >= kAttackDuration) {
		// 攻撃終了
		behavior_ = Behavior::kNormal;
		isAttack_ = false;
		attackTimer_ = 0;
		attackProgress_ = 0.0f;
		transform_.translation_ = originalPosition_;

		// 右パンチ終了時にコンボタイマーを全腕に設定
		if (currentAttackType_ == AttackType::kRightPunch && player_ != nullptr) {
			// プレイヤーの全腕にコンボタイマーを設定
			// これはPlayer側で処理する
		}

		lastAttackType_ = currentAttackType_;
		currentAttackType_ = AttackType::kNone;
	}
}

void PlayerArm::StartAttack(AttackType attackType)
{
	if (behavior_ == Behavior::kAttack) {
		return;
	}

	// 攻撃開始
	behavior_ = Behavior::kAttack;
	isAttack_ = true;
	currentAttackType_ = attackType;
	attackTimer_ = 0;
	attackProgress_ = 0.0f;

	// プレイヤーの回転を考慮して攻撃方向を計算
	Vector3 forward = { 0.0f, 0.0f, 1.0f };  // 正面方向（Z軸正方向）
	Vector3 right = { 1.0f, 0.0f, 0.0f };    // 右方向（X軸正方向）

	// ベクトルを正規化（長さを1に）
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

	// 攻撃タイプに応じて目標位置を設定
	Vector3 attackOffset = {
		forward.x * kAttackDistance,
		forward.y * kAttackDistance,
		forward.z * kAttackDistance
	};

	switch (attackType) {
	case AttackType::kRightPunch:
		// 右パンチ：右側にオフセット
		attackOffset.x += 0.7f;
		attackOffset.y += right.y * kRightPunchOffset;
		attackOffset.z += right.z * kRightPunchOffset;
		break;
	case AttackType::kLeftPunch:
		// 左パンチ：左側にオフセット
		attackOffset.x += -0.7f;
		attackOffset.y += right.y * kLeftPunchOffset;
		attackOffset.z += right.z * kLeftPunchOffset;
		break;
	}

	// 現在の位置を基準として目標位置を設定
	originalPosition_ = transform_.translation_;
	targetPosition_ = {
		originalPosition_.x + attackOffset.x,
		originalPosition_.y + attackOffset.y,
		originalPosition_.z + attackOffset.z
	};

	// 攻撃方向を保存（正規化）
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

void PlayerArm::ProcessAttack()
{
	if (!isAttack_) {
		return;
	}

	// ここで攻撃判定やエフェクトの処理を行う
	// 例：攻撃が最大前進時にダメージ判定を行う
	if (attackProgress_ >= 0.5f && attackProgress_ < 0.6f) {
		// 攻撃判定のタイミング
		// 必要に応じて敵との当たり判定やエフェクトを発生させる
	}
}

bool PlayerArm::CanCombo() const
{
	// プレイヤーの右腕が右パンチを完了していて、コンボ時間内の場合のみコンボ可能
	if (player_ != nullptr) {
		// 右腕（arms_[0]）の状態をチェック
		// この関数は左腕でのみ使用される想定
		return (!isRightArm_ && comboTimer_ > 0);
	}
	return false;
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

	// 現在の状態表示
	const char* behaviorNames[] = { "Normal", "Attack", "Skill" };
	ImGui::Text("Behavior: %s", behaviorNames[static_cast<int>(behavior_)]);

	const char* attackTypeNames[] = { "None", "RightPunch", "LeftPunch" };
	ImGui::Text("Current Attack: %s", attackTypeNames[static_cast<int>(currentAttackType_)]);
	ImGui::Text("Last Attack: %s", attackTypeNames[static_cast<int>(lastAttackType_)]);

	// タイマー表示
	ImGui::Text("Attack Timer: %d / %d", attackTimer_, kAttackDuration);
	ImGui::Text("Combo Timer: %d", comboTimer_);
	ImGui::Text("Attack Progress: %.2f", attackProgress_);

	// 攻撃可能状態
	ImGui::Text("Can Combo: %s", CanCombo() ? "Yes" : "No");
	ImGui::Text("Is Attack: %s", isAttack_ ? "Yes" : "No");

	// 位置情報
	ImGui::Text("Original Pos: (%.2f, %.2f, %.2f)",
		originalPosition_.x, originalPosition_.y, originalPosition_.z);
	ImGui::Text("Target Pos: (%.2f, %.2f, %.2f)",
		targetPosition_.x, targetPosition_.y, targetPosition_.z);
	ImGui::Text("Attack Direction: (%.2f, %.2f, %.2f)",
		attackDirection_.x, attackDirection_.y, attackDirection_.z);

	ImGui::End();
}

void PlayerArm::OnCollision(Collider* other)
{
	// 攻撃中の当たり判定処理
	if (isAttack_) {
		uint32_t typeID = other->GetTypeID();

		// 敵との当たり判定例
		if (typeID == static_cast<uint32_t>(CollisionTypeIdDef::kEnemy)) {
			// 敵にダメージを与える処理
			// Enemy* enemy = static_cast<Enemy*>(other);
			// enemy->TakeDamage(attackDamage_);
		}
	}
}

void PlayerArm::SetPlayer(Player* player)
{
	player_ = player;
	BaseObject::transform_.parent_ = &player->GetWorldTransform();
}