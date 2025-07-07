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

	// ラッシュ処理の更新
	UpdateRush();

	// コンボタイマーの更新
	UpdateComboTime();

	obj3d_->AnimationUpdate(true);
}

void PlayerArm::UpdateComboTime()
{
	if (comboTimer_ > 0) {
		comboTimer_--;
		if (comboTimer_ == 0) {
			// コンボタイマーが切れた時の処理
			comboCount_ = 0;
		}
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

		lastAttackType_ = currentAttackType_;
		currentAttackType_ = AttackType::kNone;
	}
}

void PlayerArm::UpdateRush()
{
	if (behavior_ != Behavior::kRush) {
		return;
	}

	// ラッシュタイマーの更新
	rushTimer_++;

	// ラッシュ攻撃の生成タイミングをチェック
	if (rushTimer_ % kRushInterval == 0) {
		// 新しいラッシュ攻撃を開始
		rushAttackActive_ = true;
		rushAttackTimer_ = 0;
		rushCount_++;

		// ランダムな攻撃方向を生成（左右にバリエーション）
		float randomOffset = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f; // -1.0f ~ 1.0f

		// 攻撃方向を設定
		Vector3 forward = { 0.0f, 0.0f, 1.0f };
		Vector3 attackOffset = {
			forward.x * kRushDistance + randomOffset,
			forward.y * kRushDistance,
			forward.z * kRushDistance
		};

		// 【修正】元の位置は変更せず、目標位置のみを更新
		// originalPosition_ = transform_.translation_;  // この行を削除
		targetPosition_ = {
			originalPosition_.x + attackOffset.x,
			originalPosition_.y + attackOffset.y,
			originalPosition_.z + attackOffset.z
		};

		// 攻撃方向を保存
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

	// 個別のラッシュ攻撃アニメーション
	if (rushAttackActive_) {
		rushAttackTimer_++;

		// 攻撃の進行度を計算
		float rushProgress = static_cast<float>(rushAttackTimer_) / static_cast<float>(kRushAttackDuration);
		if (rushProgress >= 1.0f) {
			rushProgress = 1.0f;
		}

		// 攻撃アニメーション（素早い往復）
		float easedProgress = 1.0f - (1.0f - rushProgress) * (1.0f - rushProgress);
		if (rushProgress > 0.6f) {
			// 早めに元の位置に戻る
			easedProgress = 1.0f - (rushProgress - 0.6f) * 2.5f;
		}

		// 現在の位置を計算
		Vector3 currentPos = {
			originalPosition_.x + (targetPosition_.x - originalPosition_.x) * easedProgress,
			originalPosition_.y + (targetPosition_.y - originalPosition_.y) * easedProgress,
			originalPosition_.z + (targetPosition_.z - originalPosition_.z) * easedProgress
		};
		transform_.translation_ = currentPos;

		// 個別攻撃終了判定
		if (rushAttackTimer_ >= kRushAttackDuration) {
			rushAttackActive_ = false;
			rushAttackTimer_ = 0;
			transform_.translation_ = originalPosition_;  // 元の位置に戻る
		}
	}

	// ラッシュ攻撃終了判定
	if (rushTimer_ >= kRushDuration) {
		// ラッシュ攻撃終了
		behavior_ = Behavior::kNormal;
		isRush_ = false;
		rushTimer_ = 0;
		rushAttackTimer_ = 0;
		rushCount_ = 0;
		rushAttackActive_ = false;
		transform_.translation_ = originalPosition_;

		// コンボリセット
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

	// 攻撃開始
	behavior_ = Behavior::kAttack;
	isAttack_ = true;
	currentAttackType_ = attackType;
	attackTimer_ = 0;
	attackProgress_ = 0.0f;

	// コンボタイマーの設定（より長い受付時間）
	if (attackType == AttackType::kRightPunch) {
		comboTimer_ = 60;  // 右パンチ後にコンボ受付開始（2秒）
		comboCount_ = 1;
	}
	else if (attackType == AttackType::kLeftPunch) {
		comboTimer_ = 90;  // 左パンチ後はラッシュ受付時間を更に延長（3秒）
		comboCount_ = 2;
	}

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

void PlayerArm::StartRush()
{
	if (behavior_ == Behavior::kAttack || behavior_ == Behavior::kRush) {
		return;
	}

	// ラッシュ攻撃開始
	behavior_ = Behavior::kRush;
	isRush_ = true;
	currentAttackType_ = AttackType::kRush;
	rushTimer_ = 0;
	rushAttackTimer_ = 0;
	rushCount_ = 0;
	rushAttackActive_ = false;

	// 初期位置を保存
	originalPosition_ = transform_.translation_;
}

void PlayerArm::ProcessAttack()
{
	if (!isAttack_ && !isRush_) {
		return;
	}

	// 通常攻撃の処理
	if (isAttack_) {
		// 攻撃が最大前進時にダメージ判定を行う
		if (attackProgress_ >= 0.5f && attackProgress_ < 0.6f) {
			// 攻撃判定のタイミング
			// 必要に応じて敵との当たり判定やエフェクトを発生させる
		}
	}

	// ラッシュ攻撃の処理
	if (isRush_ && rushAttackActive_) {
		// ラッシュ攻撃中の当たり判定
		if (rushAttackTimer_ >= 2 && rushAttackTimer_ <= 6) {
			// ラッシュ攻撃の当たり判定タイミング
			// 必要に応じて敵との当たり判定やエフェクトを発生させる
		}
	}
}

bool PlayerArm::CanCombo() const
{
	// コンボ時間内で、攻撃中でない場合にコンボ可能
	return (comboTimer_ > 0 && behavior_ == Behavior::kNormal);
}

bool PlayerArm::CanStartRush() const
{
	// 左パンチが終了していて、コンボ時間内の場合にラッシュ開始可能
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

	// 現在の状態表示
	const char* behaviorNames[] = { "Normal", "Attack", "Skill", "Rush" };
	ImGui::Text("Behavior: %s", behaviorNames[static_cast<int>(behavior_)]);

	const char* attackTypeNames[] = { "None", "RightPunch", "LeftPunch", "Rush" };
	ImGui::Text("Current Attack: %s", attackTypeNames[static_cast<int>(currentAttackType_)]);
	ImGui::Text("Last Attack: %s", attackTypeNames[static_cast<int>(lastAttackType_)]);

	// タイマー表示
	ImGui::Text("Attack Timer: %d / %d", attackTimer_, kAttackDuration);
	ImGui::Text("Combo Timer: %d", comboTimer_);
	ImGui::Text("Combo Count: %d", comboCount_);
	ImGui::Text("Attack Progress: %.2f", attackProgress_);

	// ラッシュ関連
	ImGui::Text("Rush Timer: %d / %d", rushTimer_, kRushDuration);
	ImGui::Text("Rush Count: %d", rushCount_);
	ImGui::Text("Rush Attack Active: %s", rushAttackActive_ ? "Yes" : "No");

	// 攻撃可能状態
	ImGui::Text("Can Combo: %s", CanCombo() ? "Yes" : "No");
	ImGui::Text("Can Start Rush: %s", CanStartRush() ? "Yes" : "No");
	ImGui::Text("Is Attack: %s", isAttack_ ? "Yes" : "No");
	ImGui::Text("Is Rush: %s", isRush_ ? "Yes" : "No");

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
	// 攻撃中またはラッシュ中の当たり判定処理
	if (isAttack_ || isRush_) {
		uint32_t typeID = other->GetTypeID();

		// 敵との当たり判定例
		if (typeID == static_cast<uint32_t>(CollisionTypeIdDef::kEnemy)) {
			// 敵にダメージを与える処理
			// Enemy* enemy = static_cast<Enemy*>(other);
			// if (isRush_) {
			//     enemy->TakeDamage(rushAttackDamage_);
			// } else {
			//     enemy->TakeDamage(attackDamage_);
			// }
		}
	}
}

void PlayerArm::SetPlayer(Player* player)
{
	player_ = player;
	BaseObject::transform_.parent_ = &player->GetWorldTransform();
}