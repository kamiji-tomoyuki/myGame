#include "PlayerMotionClip.h"

#include "myMath.h"
#include "Quaternion.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <json.hpp>

using json = nlohmann::json;

namespace {
	// オイラー角 → クォータニオン（scalar を .w に格納する正しい変換）。
	// 注意: エンジンの Quaternion::FromEulerAngles は成分の格納順がずれており
	//       (scalar を .x に入れてしまう) ToEulerAngles と往復すると恒等回転が
	//       X軸180度に化ける。ここでは ToEulerAngles と整合する変換を自前で行う。
	Quaternion EulerToQuat(const Vector3& e) {
		const float cr = std::cos(e.x * 0.5f), sr = std::sin(e.x * 0.5f); // roll  (X)
		const float cp = std::cos(e.y * 0.5f), sp = std::sin(e.y * 0.5f); // pitch (Y)
		const float cy = std::cos(e.z * 0.5f), sy = std::sin(e.z * 0.5f); // yaw   (Z)
		const float w = cr * cp * cy + sr * sp * sy;
		const float x = sr * cp * cy - cr * sp * sy;
		const float y = cr * sp * cy + sr * cp * sy;
		const float z = cr * cp * sy - sr * sp * cy;
		return Quaternion(x, y, z, w); // ctor は (x, y, z, w)
	}

	// X面（YZ平面）で鏡像化した姿勢を返す。
	// 反射 S=diag(-1,1,1) による回転の共役 S R S から、
	// 座標は x を反転、回転は Y/Z 成分を反転（X成分は保持）。
	PartPose MirrorPose(const PartPose& p) {
		PartPose m;
		m.translate = { -p.translate.x, p.translate.y, p.translate.z };
		m.rotate = { p.rotate.x, -p.rotate.y, -p.rotate.z };
		return m;
	}
}

namespace {
	const std::string kMotionDir = "resources/jsons/motion/";

	// Vector3 <-> json 配列
	json ToJson(const Vector3& v) { return json::array({ v.x, v.y, v.z }); }
	Vector3 ToVec3(const json& j) {
		if (j.is_array() && j.size() >= 3) {
			return Vector3{ j[0].get<float>(), j[1].get<float>(), j[2].get<float>() };
		}
		return Vector3{ 0.0f, 0.0f, 0.0f };
	}
	json ToJson(const PartPose& p) {
		json j;
		j["t"] = ToJson(p.translate);
		j["r"] = ToJson(p.rotate);
		return j;
	}
	PartPose ToPose(const json& j) {
		PartPose p;
		if (j.contains("t")) { p.translate = ToVec3(j["t"]); }
		if (j.contains("r")) { p.rotate = ToVec3(j["r"]); }
		return p;
	}
}

const PartPose& PlayerMotionClip::PoseOf(const MotionKeyframe& k, Part part) {
	switch (part) {
	case Part::kRArm: return k.rArm;
	case Part::kLArm: return k.lArm;
	case Part::kBody:
	default:          return k.body;
	}
}

// =============================================================
//  サンプリング（座標=Lerp / 回転=クォータニオンSlerp＋正規化）
// =============================================================
PartPose PlayerMotionClip::Sample(Part part, float t) const {
	if (keys_.empty()) { return PartPose{}; }
	if (keys_.size() == 1) { return PoseOf(keys_.front(), part); }

	// 端はクランプ
	if (t <= keys_.front().time) { return PoseOf(keys_.front(), part); }
	if (t >= keys_.back().time) { return PoseOf(keys_.back(), part); }

	// t を含む区間を探索
	size_t i = 0;
	for (; i + 1 < keys_.size(); ++i) {
		if (t >= keys_[i].time && t <= keys_[i + 1].time) { break; }
	}

	const MotionKeyframe& k0 = keys_[i];
	const MotionKeyframe& k1 = keys_[i + 1];
	const float span = k1.time - k0.time;
	const float u = (span > 1e-6f) ? (t - k0.time) / span : 0.0f;

	const PartPose& a = PoseOf(k0, part);
	const PartPose& b = PoseOf(k1, part);
	return BlendPose(a, b, u);
}

// =============================================================
//  2姿勢の補間（座標=線形 / 回転=クォータニオンSlerp＋正規化）
// =============================================================
PartPose PlayerMotionClip::BlendPose(const PartPose& a, const PartPose& b, float u) {
	if (u <= 0.0f) { return a; }
	if (u >= 1.0f) { return b; }
	PartPose result;
	// 座標は線形補間
	result.translate = Lerp(a.translate, b.translate, u);
	// 回転はオイラー→クォータニオン Slerp→正規化→オイラー
	Quaternion qa = EulerToQuat(a.rotate);
	Quaternion qb = EulerToQuat(b.rotate);
	Quaternion q = Quaternion::Sleap(qa, qb, u).Normalize();
	result.rotate = q.ToEulerAngles();
	return result;
}

// =============================================================
//  キーフレーム操作
// =============================================================
void PlayerMotionClip::AddKeyframe(const MotionKeyframe& key) {
	keys_.push_back(key);
	SortAndNormalize();
}

void PlayerMotionClip::UpsertKeyframe(float time, const PartPose& body, const PartPose& rArm, const PartPose& lArm, float eps) {
	// 同時刻付近のキーがあれば上書き、無ければ追加する（不変条件は内部で維持）。
	for (auto& k : keys_) {
		if (std::fabs(k.time - time) < eps) {
			k.body = body;
			k.rArm = rArm;
			k.lArm = lArm;
			SortAndNormalize();
			return;
		}
	}
	MotionKeyframe key;
	key.time = time;
	key.body = body;
	key.rArm = rArm;
	key.lArm = lArm;
	AddKeyframe(key);
}

void PlayerMotionClip::RemoveKeyframe(size_t index) {
	if (index >= keys_.size()) { return; }
	keys_.erase(keys_.begin() + index);
	SortAndNormalize();
}

void PlayerMotionClip::SortAndNormalize() {
	std::sort(keys_.begin(), keys_.end(),
		[](const MotionKeyframe& a, const MotionKeyframe& b) { return a.time < b.time; });
	// duration は最後のキー時刻以上を保証
	if (!keys_.empty()) {
		duration_ = std::max(duration_, keys_.back().time);
	}
	if (duration_ < 0.01f) { duration_ = 0.01f; }
}

// =============================================================
//  左右反転クリップの生成
// =============================================================
PlayerMotionClip PlayerMotionClip::MakeMirrored(const std::string& newName) const {
	PlayerMotionClip out;
	out.name_ = newName;
	out.duration_ = duration_;
	out.hitStart_ = hitStart_;
	out.hitEnd_ = hitEnd_;
	out.damage_ = damage_;
	out.comboWindowStart_ = comboWindowStart_;

	// ヒット腕を反転（両方はそのまま）
	if (hitArm_ == HitArm::kRight) { out.hitArm_ = HitArm::kLeft; }
	else if (hitArm_ == HitArm::kLeft) { out.hitArm_ = HitArm::kRight; }
	else { out.hitArm_ = HitArm::kBoth; }

	// 各キー: 体は鏡像化、右腕と左腕を入れ替えつつ鏡像化
	out.keys_.reserve(keys_.size());
	for (const auto& k : keys_) {
		MotionKeyframe mk;
		mk.time = k.time;
		mk.body = MirrorPose(k.body);
		mk.rArm = MirrorPose(k.lArm); // 左腕→右腕
		mk.lArm = MirrorPose(k.rArm); // 右腕→左腕
		out.keys_.push_back(mk);
	}
	out.SortAndNormalize();
	return out;
}

// =============================================================
//  戦闘メタ判定（正規化時刻で比較）
// =============================================================
bool PlayerMotionClip::IsInHitWindow(float t) const {
	if (duration_ <= 0.0f) { return false; }
	const float n = t / duration_;
	return (n >= hitStart_ && n <= hitEnd_);
}

bool PlayerMotionClip::IsComboWindowOpen(float t) const {
	if (duration_ <= 0.0f) { return false; }
	const float n = t / duration_;
	return (n >= comboWindowStart_);
}

// =============================================================
//  JSON 保存 / 読込
// =============================================================
bool PlayerMotionClip::Save() const {
	try {
		std::filesystem::create_directories(kMotionDir);
		json root;
		root["name"] = name_;
		root["duration"] = duration_;
		root["hitStart"] = hitStart_;
		root["hitEnd"] = hitEnd_;
		root["hitArm"] = static_cast<int>(hitArm_);
		root["damage"] = damage_;
		root["comboWindowStart"] = comboWindowStart_;

		root["keys"] = json::array();
		for (const auto& k : keys_) {
			json jk;
			jk["time"] = k.time;
			jk["body"] = ToJson(k.body);
			jk["rArm"] = ToJson(k.rArm);
			jk["lArm"] = ToJson(k.lArm);
			root["keys"].push_back(jk);
		}

		std::ofstream ofs(kMotionDir + name_ + ".json");
		if (!ofs) { return false; }
		ofs << root.dump(2);
		return true;
	}
	catch (...) {
		return false;
	}
}

bool PlayerMotionClip::Load(const std::string& name) {
	try {
		std::ifstream ifs(kMotionDir + name + ".json");
		if (!ifs) { return false; }
		json root;
		ifs >> root;

		name_ = root.value("name", name);
		duration_ = root.value("duration", 1.0f);
		hitStart_ = root.value("hitStart", 0.3f);
		hitEnd_ = root.value("hitEnd", 0.6f);
		hitArm_ = static_cast<HitArm>(root.value("hitArm", 0));
		damage_ = root.value("damage", 50u);
		comboWindowStart_ = root.value("comboWindowStart", 0.55f);

		keys_.clear();
		if (root.contains("keys") && root["keys"].is_array()) {
			for (const auto& jk : root["keys"]) {
				MotionKeyframe k;
				k.time = jk.value("time", 0.0f);
				if (jk.contains("body")) { k.body = ToPose(jk["body"]); }
				if (jk.contains("rArm")) { k.rArm = ToPose(jk["rArm"]); }
				if (jk.contains("lArm")) { k.lArm = ToPose(jk["lArm"]); }
				keys_.push_back(k);
			}
		}
		SortAndNormalize();
		return true;
	}
	catch (...) {
		return false;
	}
}
