#include "PlayerAttack.h"
#include "Player.h"
#include "Input.h"
#include <Arm/PlayerArm.h>

PlayerAttack::PlayerAttack(Player* player,
	const std::array<std::unique_ptr<PlayerArm>, 2>& arms)
	: player_(player)
	, arms_(&arms)
{
}

// =============================================================
// Update
// =============================================================
void PlayerAttack::Update()
{
	const auto& a = *arms_;

	if (!Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		return;
	}

	// --- ラッシュ優先 ---
	if (a[kLArm] && a[kLArm]->CanStartRush()) {
		if (a[kRArm]) { a[kRArm]->StartRush(); }
		if (a[kLArm]) { a[kLArm]->StartRush(); }
		return;
	}

	// --- 左パンチコンボ ---
	if (a[kRArm] && a[kRArm]->CanCombo() &&
		a[kRArm]->GetLastAttackType() == PlayerArm::AttackType::kRightPunch) {
		if (a[kLArm]) {
			a[kLArm]->StartAttack(PlayerArm::AttackType::kLeftPunch);
		}
		return;
	}

	// --- 右パンチ（初撃） ---
	{
		bool anyAttacking = false;
		for (const auto& arm : *arms_) {
			if (arm->GetBehavior() == PlayerArm::Behavior::kAttack ||
				arm->GetBehavior() == PlayerArm::Behavior::kRush) {
				anyAttacking = true;
				break;
			}
		}

		if (!anyAttacking && a[kRArm]) {
			a[kRArm]->StartAttack(PlayerArm::AttackType::kRightPunch);
		}
	}
}

// =============================================================
// UI表示判定
// =============================================================
bool PlayerAttack::CanRightPunch() const
{
	const auto& a = *arms_;

	if (player_->IsDodging() || player_->IsHitReacting()) {
		return false;
	}

	// ラッシュ待機中なら右パンチUI非表示
	if (a[kLArm] && a[kLArm]->CanStartRush()) {
		return false;
	}

	// 左パンチコンボ待機中なら右パンチUI非表示
	if (a[kRArm] &&
		a[kRArm]->CanCombo() &&
		a[kRArm]->GetLastAttackType() == PlayerArm::AttackType::kRightPunch) {
		return false;
	}

	// 右パンチ実行中なら表示
	if (IsRightPunchActive()) {
		return true;
	}

	// 何も攻撃していなければ右パンチ可能
	bool anyAttacking = false;
	for (const auto& arm : *arms_) {
		if (arm->GetBehavior() == PlayerArm::Behavior::kAttack ||
			arm->GetBehavior() == PlayerArm::Behavior::kRush) {
			anyAttacking = true;
			break;
		}
	}
	return !anyAttacking;
}

bool PlayerAttack::CanLeftPunch() const
{
	const auto& a = *arms_;

	if (player_->IsDodging() || player_->IsHitReacting()) {
		return false;
	}

	if (a[kLArm] && a[kLArm]->CanStartRush()) {
		return false;
	}

	if (IsLeftPunchActive()) {
		return true;
	}

	// 右パンチ後コンボ待機中
	if (a[kRArm] &&
		a[kRArm]->CanCombo() &&
		a[kRArm]->GetLastAttackType() == PlayerArm::AttackType::kRightPunch) {
		return true;
	}

	return false;
}

bool PlayerAttack::CanRush() const
{
	const auto& a = *arms_;

	if (player_->IsDodging() || player_->IsHitReacting()) {
		return false;
	}

	if (IsRushActive()) {
		return true;
	}

	if (a[kLArm] && a[kLArm]->CanStartRush()) {
		return true;
	}

	return false;
}

// =============================================================
// 攻撃実行中判定
// =============================================================
bool PlayerAttack::IsRightPunchActive() const
{
	const auto& a = *arms_;
	return a[kRArm] &&
		a[kRArm]->GetBehavior() == PlayerArm::Behavior::kAttack &&
		a[kRArm]->GetCurrentAttackType() == PlayerArm::AttackType::kRightPunch;
}

bool PlayerAttack::IsLeftPunchActive() const
{
	const auto& a = *arms_;
	return a[kLArm] &&
		a[kLArm]->GetBehavior() == PlayerArm::Behavior::kAttack &&
		a[kLArm]->GetCurrentAttackType() == PlayerArm::AttackType::kLeftPunch;
}

bool PlayerAttack::IsRushActive() const
{
	for (const auto& arm : *arms_) {
		if (arm->GetBehavior() == PlayerArm::Behavior::kRush) {
			return true;
		}
	}
	return false;
}