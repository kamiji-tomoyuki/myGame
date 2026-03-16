#pragma once

class Player;

/// <summary>
/// プレイヤー行動の基底インターフェース（State Pattern）
/// Playing状態内のBehavior（Root / Dodge / HitReact）に対応
/// </summary>
class IPlayerBehavior
{
public:
	virtual ~IPlayerBehavior() = default;

	/// <summary>行動開始時に1度だけ呼ばれる</summary>
	virtual void Enter(Player* player) {}

	/// <summary>
	/// 毎フレーム呼ばれる。
	/// 次の行動を返す（nullptr なら自分自身を継続）
	/// </summary>
	virtual IPlayerBehavior* Update(Player* player) = 0;

	/// <summary>行動終了時に1度だけ呼ばれる</summary>
	virtual void Exit(Player* player) {}
};
