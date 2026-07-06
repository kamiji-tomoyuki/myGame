#pragma once
#include "Vector3.h"

using namespace Engine;
class Player;

/// <summary>
/// ゲームクリア時の演出を管理するクラス
/// ジャンプしながら回転する無限ループ演出
/// </summary>
class PlayerGameClearEffect
{
public:
	explicit PlayerGameClearEffect(Player* player);

	/// <summary>
	/// 初回呼び出し時に開始位置を記録し、毎フレーム演出を進行
	/// </summary>
	void Update();

private:
	Player* player_ = nullptr;

	/// 演出開始位置（初回Update時に記録）
	Vector3 clearStartPos_{};

	/// 経過フレーム数
	float timer_ = 0.0f;

	/// 初回Update済みか
	bool initialized_ = false;

	// --- 定数 ---
	/// 円周率
	static inline const float kPi_ = 3.14159265f;

	/// ジャンプ1サイクルのフレーム数
	static inline const float kJumpCycle_ = 40.0f;

	/// ジャンプの高さ
	static inline const float kJumpHeight_ = 2.0f;

	/// 毎フレームの回転加算量（rad）
	static inline const float kRotateSpeed_ = 0.05f;

	/// タイマーのオーバーフロー防止リセット閾値
	static inline const float kTimerResetThreshold_ = kJumpCycle_ * 1000.0f;
};