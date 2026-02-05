#pragma once

class Player;

/// <summary>
/// ゲームオーバー時の演出を管理するクラス
/// 回転しながら縮小し、サイズが 0 以下になった時に isAlive を false にする
/// </summary>
class PlayerGameOverEffect
{
public:
	explicit PlayerGameOverEffect(Player* player);

	/// <summary>
	/// 毎フレーム更新。演出完了後も呼び続けて問題ない。
	/// </summary>
	void Update();

	/// <summary>
	/// プレイヤーの生存フラグを返す（演出完了で false）
	/// </summary>
	bool IsAlive() const { return isAlive_; }

private:
	Player* player_ = nullptr;

	/// 演出中のプレイヤーが「生きている」か
	bool isAlive_ = true;

	/// 毎フレームの回転加算量（rad）
	static constexpr float kRotateSpeed_ = 0.5f;

	/// 毎フレームのスケール減少量
	static constexpr float kShrinkSpeed_ = 0.02f;
};