#pragma once
#include "Vector3.h"

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
	
	// --- 定数 ---
	/// ゼロ→一へのLerp所要時間（秒）
	static inline const float   kDuration_ = 1.5f;

	/// 1フレームの経過時間（秒）。60fps 固定前提
	static inline const float   kFrameDeltaTime_ = 1.0f / 60.0f;

	/// スケール補間の開始値（ゼロスケール）
	static inline const Vector3 kScaleStart_ = { 0.0f, 0.0f, 0.0f };

	/// スケール補間の終了値（等倍）
	static inline const Vector3 kScaleEnd_ = { 1.0f, 1.0f, 1.0f };
};