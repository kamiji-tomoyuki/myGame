#pragma once
#include "ViewProjection.h"
#include <memory>

using namespace Engine;
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

	/// <summary>毎フレーム呼ばれる。次の状態を返す（nullptr なら遷移なし）</summary>
	virtual std::unique_ptr<IPlayerState> Update(Player* player) = 0;

	/// <summary>状態から出るときに1度だけ呼ばれる</summary>
	virtual void Exit(Player* player) {}
};
