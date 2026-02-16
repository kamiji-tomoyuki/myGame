#pragma once
#include "Vector3.h"  // プロジェクト側の Vector3 ヘッダー

class Player;

/// <summary>
/// プレイヤーの開始時演出を管理するクラス
/// スケールを 0→1 に補間し、完了時に isEnd を立てる
/// </summary>
class PlayerStartEffect
{
public:
	explicit PlayerStartEffect(Player* player);

	/// <summary>
	/// 毎フレーム更新。演出完了後も呼び続けて問題ない。
	/// </summary>
	void Update();

	/// <summary>
	/// 演出が完了したか
	/// </summary>
	bool IsEnd() const { return isEnd_; }

private:
	Player* player_ = nullptr;

	/// 補間パラメータ（0.0〜1.0）
	float easeT_ = 0.0f;

	/// 演出完了フラグ
	bool isEnd_ = false;

	/// ゼロ→一へのLerp所要時間（秒）
	static constexpr float kDuration_ = 1.5f;
};