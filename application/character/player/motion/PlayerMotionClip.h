#pragma once
#include "Vector3.h"
#include <cstdint>
#include <string>
#include <vector>

using namespace Engine;

/// <summary>
/// 各パートの姿勢（座標・回転のみ）
/// </summary>
struct PartPose {
	Vector3 translate = { 0.0f, 0.0f, 0.0f };
	Vector3 rotate = { 0.0f, 0.0f, 0.0f };
};

/// <summary>
/// キーフレーム（時刻＋体/右腕/左腕の姿勢）
/// </summary>
struct MotionKeyframe {
	float    time = 0.0f; // 秒
	PartPose body;
	PartPose rArm;
	PartPose lArm;
};

/// <summary>ヒット対象腕</summary>
enum class HitArm {
	kRight,
	kLeft,
	kBoth,
};

/// <summary>
/// プレイヤー攻撃モーションクリップ。
/// 体・右腕・左腕のキーフレームを保持し、時刻で補間サンプリングする。
/// 回転補間はクォータニオン Slerp＋正規化を用いる。
/// </summary>
class PlayerMotionClip {
public:
	// パートID（サンプリング指定用）
	enum class Part { kBody, kRArm, kLArm };

	/// <summary>
	/// 2姿勢の補間（座標=線形, 回転=クォータニオンSlerp＋正規化）。
	/// クリップ間の補間（ブレンド）にも使う共通関数。
	/// </summary>
	static PartPose BlendPose(const PartPose& a, const PartPose& b, float u);

	// --- サンプリング（t: 秒。[0, duration] にクランプ） ---
	PartPose Sample(Part part, float t) const;
	PartPose SampleBody(float t) const { return Sample(Part::kBody, t); }
	PartPose SampleRArm(float t) const { return Sample(Part::kRArm, t); }
	PartPose SampleLArm(float t) const { return Sample(Part::kLArm, t); }

	// --- JSON 保存 / 読込（resources/jsons/motion/<name>.json） ---
	bool Save() const;
	bool Load(const std::string& name);

	// --- キーフレーム操作（内部ベクタの不変条件=昇順ソート/duration整合はクラス内で維持） ---
	void AddKeyframe(const MotionKeyframe& key); // time昇順を維持して挿入
	// 指定時刻付近(±eps)のキーを更新し、無ければ追加する（エディタのキー登録用）
	void UpsertKeyframe(float time, const PartPose& body, const PartPose& rArm, const PartPose& lArm, float eps = 0.01f);
	// 指定インデックスのキーを削除（範囲外は無視）
	void RemoveKeyframe(size_t index);
	void SortAndNormalize();                     // time昇順ソート＋duration整合

	/// <summary>
	/// 左右反転したクリップを生成する（右パンチ↔左パンチを同一挙動にするため）。
	/// 各パートをX面で鏡像化し、右腕と左腕を入れ替える。ヒット腕もR↔Lに反転。
	/// </summary>
	PlayerMotionClip MakeMirrored(const std::string& newName) const;

	// --- 戦闘メタ（正規化時刻 [0,1] 基準） ---
	bool  IsInHitWindow(float t) const;      // t秒がヒット窓内か
	bool  IsComboWindowOpen(float t) const;  // t秒がコンボ受付開始以降か

	// --- getter / setter ---
	const std::string& GetName() const { return name_; }
	void  SetName(const std::string& n) { name_ = n; }
	float GetDuration() const { return duration_; }
	void  SetDuration(float d) { duration_ = (d > 0.01f) ? d : 0.01f; }
	// 読み取り専用アクセス（表示・反復用）。内部ベクタは直接書き換えさせない。
	const std::vector<MotionKeyframe>& Keys() const { return keys_; }
	size_t KeyCount() const { return keys_.size(); }

	float    GetHitStart() const { return hitStart_; }
	float    GetHitEnd() const { return hitEnd_; }
	HitArm   GetHitArm() const { return hitArm_; }
	uint32_t GetDamage() const { return damage_; }
	float    GetComboWindowStart() const { return comboWindowStart_; }

	void SetHitStart(float v) { hitStart_ = v; }
	void SetHitEnd(float v) { hitEnd_ = v; }
	void SetHitArm(HitArm v) { hitArm_ = v; }
	void SetDamage(uint32_t v) { damage_ = v; }
	void SetComboWindowStart(float v) { comboWindowStart_ = v; }

private:
	// メンバ選択（Partに対応する MotionKeyframe 内の PartPose を返す）
	static const PartPose& PoseOf(const MotionKeyframe& k, Part part);

	std::string name_ = "newMotion";
	float duration_ = 1.0f; // 秒
	std::vector<MotionKeyframe> keys_;

	// 戦闘メタ（正規化時刻 [0,1]）
	float    hitStart_ = 0.3f;
	float    hitEnd_ = 0.6f;
	HitArm   hitArm_ = HitArm::kRight;
	uint32_t damage_ = 50;
	float    comboWindowStart_ = 0.55f;
};
