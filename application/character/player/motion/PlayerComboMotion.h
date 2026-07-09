#pragma once
#include "PlayerMotionClip.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

using namespace Engine;

/// <summary>
/// 作成した攻撃モーションクリップを順に再生し、入力で連鎖させるコンボ制御。
/// 1コンボ段 = 1クリップ。resources/jsons/motion/combo.json のクリップ順を再生する。
/// クリップ間は次クリップの開始姿勢へブレンド補間する（時間は combo.json で設定）。
/// </summary>
class PlayerComboMotion {
public:
	/// <summary>入力(TryAdvance)の結果</summary>
	enum class Result {
		kNone,     // 何も起きない
		kAttacked, // コンボ開始/次段へ
		kRush,     // コンボ完走 → ラッシュへ（呼び出し側でラッシュ開始）
	};

	/// <summary>combo.json とクリップを読み込む（無ければ既定クリップを生成）</summary>
	void Init();

	/// <summary>
	/// 単一クリップ（フィニッシャー等）を読み込む。combo_listは使わない。
	/// 無ければ既定フィニッシャークリップを生成。autoAdvance/loopはoffにする。
	/// </summary>
	void InitSingle(const std::string& clipName);

	/// <summary>攻撃入力時に呼ぶ</summary>
	Result TryAdvance();

	/// <summary>毎フレーム更新</summary>
	void Update(float dt);

	/// <summary>再生停止（ラッシュ移行時など）</summary>
	void Stop();

	/// <summary>先頭クリップから再生開始（エディタのコンボプレビュー用）</summary>
	void StartFromBeginning();

	bool IsActive() const { return phase_ != Phase::kIdle; }
	// 最終段まで繋いだバッファ入力によるラッシュ要求を取り出す（true=呼び出し側がラッシュ開始）
	bool ConsumePendingRush() { const bool r = pendingRush_; pendingRush_ = false; return r; }

	// 現在時刻の各パート姿勢
	PartPose GetBodyPose() const;
	PartPose GetRArmPose() const;
	PartPose GetLArmPose() const;

	// ヒット
	bool     IsHitActive() const; // 現clipのヒット窓内 かつ 未ヒット
	HitArm   GetHitArm() const;
	uint32_t GetDamage() const;
	void     MarkHit() { hasHitThisClip_ = true; }

	int GetClipCount() const { return static_cast<int>(clips_.size()); }
	int GetCurrentIndex() const { return index_; } // 再生中クリップの段（エディタ表示用）

	// --- 設定 ---
	void  SetBlendTime(float t) { blendTime_ = (t < 0.0f) ? 0.0f : t; }
	float GetBlendTime() const { return blendTime_; }
	void  SetAutoAdvance(bool v) { autoAdvance_ = v; } // 入力無しで自動連鎖（プレビュー）
	void  SetLoop(bool v) { loop_ = v; }               // 自動連鎖時にループ

private:
	// 再生フェーズ
	enum class Phase {
		kIdle,   // 停止
		kClip,   // クリップ再生中
		kBlend,  // クリップ間ブレンド中
		kReturn, // 終端姿勢→基準姿勢へ戻すブレンド中（コンボ非継続時）
	};

	void StartClip(int index);
	void BeginBlend(int nextIndex); // 現クリップ終端姿勢→次クリップ開始姿勢へ
	void BeginReturn();             // 現クリップ終端姿勢→基準姿勢へ（コンボ非継続時）
	int  ResolveNextIndex() const;  // 次に再生すべきインデックス（無ければ-1）
	void LoadDefaults();            // 既定コンボ（右パンチ→左パンチ）を生成
	void LoadDefaultFinisher();     // 既定フィニッシャー（振りかぶり→大パンチ→戻り）を生成
	bool LoadFromFiles();           // combo.json からクリップ＋補間時間を読み込む

	std::vector<PlayerMotionClip> clips_;

	Phase phase_ = Phase::kIdle;
	int   index_ = -1;         // 現在クリップ（kClip）/ ブレンド元クリップ（kBlend）
	int   pendingNext_ = -1;   // ブレンド先クリップ（kBlend）
	float time_ = 0.0f;        // クリップ再生時刻
	float blendTimer_ = 0.0f;  // ブレンド経過時間
	std::array<PartPose, 3> blendFrom_{}; // 0=体 1=右腕 2=左腕（ブレンド元姿勢）

	bool  queuedNext_ = false;
	bool  hasHitThisClip_ = false;
	// 攻撃入力バッファ：受付窓(comboWindowStart)より前に押しても捨てずに保持し、
	// 窓に達した瞬間に「次段へキャンセル連鎖」／「最終段→ラッシュ要求」を解決する。
	// → フレーム完全一致や押しっぱなしを要求せず、モーション中に押せば確実に繋がる。
	bool  bufferedInput_ = false;
	bool  pendingRush_ = false; // 最終段でバッファ入力が窓に達したラッシュ要求（呼び出し側が消費）

	float blendTime_ = 0.1f;   // クリップ間の補間時間（秒）
	bool  autoAdvance_ = false;
	bool  loop_ = false;

	// 腕の基準位置（実プレイヤーと同値）
	const Vector3 kRArmBase_ = { 1.7f, 0.0f, 1.3f };
	const Vector3 kLArmBase_ = { -1.7f, 0.0f, 1.3f };
};
