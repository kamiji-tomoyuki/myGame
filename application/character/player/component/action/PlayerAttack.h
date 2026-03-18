#pragma once
#include <array>
#include <memory>
#include "GlobalVariables.h"

class Player;
class PlayerArm;

/// <summary>
/// プレイヤーの攻撃ロジックを管理するクラス
/// UpdateAttack・Can/IsXXX の一切を負担する
/// </summary>
class PlayerAttack
{
public:
	PlayerAttack(Player* player, const std::array<std::unique_ptr<PlayerArm>, 2>& arms);

	void Update();

	bool CanRightPunch() const;
	bool CanLeftPunch()  const;
	bool CanRush()       const;

	bool IsRightPunchActive() const;
	bool IsLeftPunchActive()  const;
	bool IsRushActive()       const;

private:
	void ApplyVariables();

private:
	Player* player_ = nullptr;
	const std::array<std::unique_ptr<PlayerArm>, 2>* arms_ = nullptr;

	static constexpr int kRArm = 0;
	static constexpr int kLArm = 1;

	bool  comboProtected_ = false;
	int   comboProtectTimer_ = 0;

	// GlobalVariables で調整可能な変数
	int kComboProtectDuration_ = 25; // 保護フレーム数

	GlobalVariables* variables_ = nullptr;
	static const std::string kGroupName_;
};