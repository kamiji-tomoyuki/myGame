#pragma once
#include "ViewProjection.h"

class Player;

/// <summary>
/// プレイヤー状態の基底インターフェース（State Pattern）
/// GameState（Playing / GameOver / GameClear）に対応
/// </summary>
class IPlayerState
{
public:
	virtual ~IPlayerState() = default;

	/// <summary>状態に入ったときに1度だけ呼ばれる</summary>
	virtual void Enter(Player* player) = 0;

	/// <summary>毎フレーム呼ばれる。次の状態を返す（自分自身なら遷移なし）</summary>
	virtual IPlayerState* Update(Player* player) = 0;

	/// <summary>状態から出るときに1度だけ呼ばれる</summary>
	virtual void Exit(Player* player) {}
};
