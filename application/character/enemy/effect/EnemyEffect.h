#pragma once
#include "Vector3.h"
#include "ViewProjection.h"

class Enemy;

/// <summary>
/// 敵の各種演出（開始落下・ゲームオーバー・ゲームクリア）を担うコンポーネント
/// Player の PlayerStartEffect / PlayerGameOverEffect / PlayerGameClearEffect に対応
/// ※ GameOver / GameClear の更新ロジックは State クラスに移譲済み。
///    このクラスは UpdateStartEffect のみ保持し、演出に必要な
///    パラメータ（タイマー・座標）を集約する。
/// </summary>
class EnemyEffect
{
public:
	EnemyEffect() = default;

	/// <summary>落下演出の更新（ゲーム開始前に毎フレーム呼ぶ）</summary>
	void UpdateStartEffect(Enemy* enemy);

	/// <summary>落下が完了したか</summary>
	bool IsFallComplete() const { return isFallComplete_; }

private:
	float   fallTimer_    = 0.0f;
	bool    isFallComplete_ = false;

	static constexpr float  kFallDuration_ = 60.0f;
	const Vector3 fallStartPos_ = { 0.0f, 10.0f, 15.0f };
	const Vector3 fallEndPos_   = { 0.0f,  0.0f, 15.0f };
};
