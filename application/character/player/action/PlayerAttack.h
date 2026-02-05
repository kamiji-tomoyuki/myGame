#pragma once
#include <array>
#include <memory>

class Player;
class PlayerArm;

/// <summary>
/// プレイヤーの攻撃ロジックを管理するクラス
/// UpdateAttack・Can/IsXXX の一切を負担する
/// </summary>
class PlayerAttack
{
public:
	/// <summary>
	/// @param player   オーナープレイヤー
	/// @param arms     Player側で持つ arms_ の参照を受け取る
	/// </summary>
	PlayerAttack(Player* player, const std::array<std::unique_ptr<PlayerArm>, 2>& arms);

	/// <summary>
	/// 毎フレーム入力チェックと攻撃開始
	/// </summary>
	void Update();

	// -------------------------------------------------------
	// UI表示判定
	// -------------------------------------------------------
	bool CanRightPunch() const;
	bool CanLeftPunch() const;
	bool CanRush() const;

	// -------------------------------------------------------
	// 攻撃実行中判定
	// -------------------------------------------------------
	bool IsRightPunchActive() const;
	bool IsLeftPunchActive() const;
	bool IsRushActive() const;

private:
	Player* player_ = nullptr;

	/// Player側 arms_ へのポインタ（読み取りのみ）
	const std::array<std::unique_ptr<PlayerArm>, 2>* arms_ = nullptr;

	// --- 腕インデックス定数（Player::ModelArm と対応） ---
	static constexpr int kRArm = 0;
	static constexpr int kLArm = 1;
};