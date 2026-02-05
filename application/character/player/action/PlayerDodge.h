#pragma once
#include "Vector3.h"

class Player;
class StageManager;

/// <summary>
/// プレイヤーの回避を管理するクラス
/// StartDodge・UpdateDodge を負担し、回避中フラグも提供する
/// </summary>
class PlayerDodge
{
public:
	/// <summary>
	/// @param player        オーナープレイヤー
	/// @param stageManager  ステージ境界チェック用
	/// </summary>
	PlayerDodge(Player* player, StageManager* stageManager);

	/// <summary>
	/// 回避を開始する。移動方向（ローカル空間）を渡す。
	/// </summary>
	void Start(const Vector3& localDirection);

	/// <summary>
	/// 毎フレーム更新。回避中でない場合は何もしない。
	/// </summary>
	void Update();

	/// <summary>
	/// 現在回避中か
	/// </summary>
	bool IsDodging() const { return isDodging_; }

private:
	Player* player_ = nullptr;
	StageManager* stageManager_ = nullptr;

	/// 回避中フラグ
	bool isDodging_ = false;

	/// 経過フレーム
	int timer_ = 0;

	/// ワールド空間での回避移動方向（正規化済み）
	Vector3 dodgeDirection_{};

	/// 回避開始時の回転（Y軸を保持するため）
	Vector3 startRotation_{};

	/// 傾き方向の回転量
	Vector3 tiltRotation_{};

	// --- 定数 ---
	/// 回避の総フレーム数
	static constexpr int kDuration_ = 30;

	/// 毎フレームの移動距離
	static constexpr float kSpeed_ = 0.3f;

	/// 傾き角度（rad）
	static constexpr float kTiltAngle_ = 0.52f;

	/// 傾き入りフレーム数
	static constexpr int kTiltInDuration_ = 10;

	/// 傾き出フレーム数
	static constexpr int kTiltOutDuration_ = 10;

	// ---------------------------------------------------
	// 内部ヘルパー
	// ---------------------------------------------------

	/// 移動方向からワールド空間の傾き回転を計算してセットする
	void CalcTiltRotation(const Vector3& worldDirection);
};