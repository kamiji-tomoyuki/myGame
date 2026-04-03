#pragma once
#include <memory>

class Enemy;

/// <summary>
/// 敵の状態インターフェース (State Pattern)
/// PlayerのIPlayerStateに対応
/// </summary>
class IEnemyState
{
public:
	virtual ~IEnemyState() = default;

	/// <summary>状態に入ったときに一度だけ呼ばれる</summary>
	virtual void Enter(Enemy* enemy) {}

	/// <summary>
	/// 毎フレーム呼ばれる更新処理
	/// 状態遷移が必要な場合は次の状態のunique_ptrを返す
	/// 遷移不要なら nullptr を返す
	/// </summary>
	virtual std::unique_ptr<IEnemyState> Update(Enemy* enemy) = 0;

	/// <summary>状態から出るときに一度だけ呼ばれる</summary>
	virtual void Exit(Enemy* enemy) {}
};