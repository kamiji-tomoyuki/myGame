#include "PlayerComboMotion.h"

#include <filesystem>
#include <fstream>
#include <json.hpp>

using json = nlohmann::json;

namespace {
	const std::string kMotionDir = "resources/jsons/motion/";
}

void PlayerComboMotion::Init() {
	clips_.clear();
	if (!LoadFromFiles() || clips_.empty()) {
		LoadDefaults();
	}
	Stop();
}

// 単一クリップ再生（フィニッシャー用）
void PlayerComboMotion::InitSingle(const std::string& clipName) {
	clips_.clear();
	autoAdvance_ = false;
	loop_ = false;
	PlayerMotionClip clip;
	if (clip.Load(clipName)) {
		clips_.push_back(clip);
	} else {
		LoadDefaultFinisher(); // ファイルが無ければ既定フィニッシャーを生成
	}
	Stop();
}

// 既定フィニッシャー（右腕：振りかぶり→大パンチ→戻り、体は右→左へひねる）
void PlayerComboMotion::LoadDefaultFinisher() {
	clips_.clear();
	PlayerMotionClip clip;
	clip.SetName("finisher");
	clip.SetDuration(0.75f);
	clip.SetHitStart(0.45f);
	clip.SetHitEnd(0.62f);
	clip.SetHitArm(HitArm::kRight);
	clip.SetDamage(150);
	clip.SetComboWindowStart(1.1f); // 連鎖不可（単発）

	auto key = [&](float t, const Vector3& bodyRot, const Vector3& rArmT, const Vector3& rArmR) {
		MotionKeyframe k;
		k.time = t;
		k.body.rotate = bodyRot;
		k.rArm.translate = rArmT;
		k.rArm.rotate = rArmR;
		k.lArm.translate = kLArmBase_;
		clip.Keys().push_back(k);
	};

	// 振りかぶり（体を右へ、右腕を後方＋右へ引く）
	key(0.00f, { 0.0f,  0.0f, 0.0f }, kRArmBase_,                    { 0.0f,  0.0f, 0.0f });
	key(0.28f, { 0.0f,  0.5f, 0.0f }, { 2.4f, 0.1f, -0.4f },         { 0.0f,  0.6f, 0.0f });
	// 大パンチ（体を左へ、右腕を大きく前方へ）
	key(0.52f, { 0.0f, -0.6f, 0.0f }, { 0.2f, 0.1f,  4.2f },         { 0.0f, -1.2f, 0.0f });
	// 戻り
	key(0.75f, { 0.0f,  0.0f, 0.0f }, kRArmBase_,                    { 0.0f,  0.0f, 0.0f });
	clip.SortAndNormalize();
	clips_.push_back(clip);
}

// コンボ定義ファイルからクリップ＋補間時間を読み込む。
// コンボ定義は clip の保存ファイル(<name>.json)と衝突しないよう専用ファイル combo_list.json を使う。
// 互換: combo_list.json が無ければ旧 combo.json を試すが、それが clip(=keysを持つ)なら無視する。
// 対応形式:
//   配列:       ["punchR", "punchL"]
//   オブジェクト: { "clips": ["punchR","punchL"], "blendTime": 0.1 }
bool PlayerComboMotion::LoadFromFiles() {
	auto tryLoad = [&](const std::string& file) -> bool {
		std::ifstream ifs(kMotionDir + file);
		if (!ifs) { return false; }
		json root;
		try { ifs >> root; } catch (...) { return false; }

		const json* names = nullptr;
		if (root.is_array()) {
			names = &root;
		}
		else if (root.is_object()) {
			// clip ファイル（keys を持つ）はコンボ定義ではないので無視
			if (root.contains("keys")) { return false; }
			blendTime_ = root.value("blendTime", blendTime_);
			if (root.contains("clips") && root["clips"].is_array()) {
				names = &root["clips"];
			}
		}
		if (!names) { return false; }

		for (const auto& n : *names) {
			PlayerMotionClip clip;
			if (clip.Load(n.get<std::string>())) {
				clips_.push_back(clip);
			}
		}
		return !clips_.empty();
	};

	try {
		if (tryLoad("combo_list.json")) { return true; }
		clips_.clear();
		return tryLoad("combo.json"); // 後方互換（clipなら内部でfalse）
	}
	catch (...) {
		return false;
	}
}

// 既定コンボ（右パンチ → 左パンチ）を手続き的に生成
void PlayerComboMotion::LoadDefaults() {
	clips_.clear();

	auto makePunch = [&](const std::string& name, bool right) {
		PlayerMotionClip clip;
		clip.SetName(name);
		clip.SetDuration(0.35f);
		clip.SetHitStart(0.35f);
		clip.SetHitEnd(0.65f);
		clip.SetHitArm(right ? HitArm::kRight : HitArm::kLeft);
		clip.SetDamage(50);
		clip.SetComboWindowStart(0.5f);

		const Vector3 base = right ? kRArmBase_ : kLArmBase_;
		const Vector3 extend = right ? Vector3{ 0.9f, 0.15f, 3.4f } : Vector3{ -0.9f, 0.15f, 3.4f };
		const Vector3 wind = right ? Vector3{ 2.0f, 0.0f, 0.4f } : Vector3{ -2.0f, 0.0f, 0.4f };

		auto key = [&](float t, const Vector3& punchArm, const Vector3& punchRot, const Vector3& bodyRot) {
			MotionKeyframe k;
			k.time = t;
			k.body.rotate = bodyRot;
			if (right) {
				k.rArm.translate = punchArm; k.rArm.rotate = punchRot;
				k.lArm.translate = kLArmBase_;
			} else {
				k.lArm.translate = punchArm; k.lArm.rotate = punchRot;
				k.rArm.translate = kRArmBase_;
			}
			clip.Keys().push_back(k);
		};

		const float twist = right ? 0.25f : -0.25f;
		key(0.00f, wind, { -0.3f, 0.0f, 0.0f }, { 0.0f, -twist, 0.0f }); // 引き
		key(0.15f, extend, { -0.6f, 0.0f, 0.0f }, { 0.0f, twist, 0.0f }); // 打点
		key(0.35f, base, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f });     // 戻り
		clip.SortAndNormalize();
		return clip;
	};

	clips_.push_back(makePunch("punchR", true));
	clips_.push_back(makePunch("punchL", false));
}

void PlayerComboMotion::StartClip(int index) {
	phase_ = Phase::kClip;
	index_ = index;
	pendingNext_ = -1;
	time_ = 0.0f;
	blendTimer_ = 0.0f;
	queuedNext_ = false;
	hasHitThisClip_ = false;
}

void PlayerComboMotion::StartFromBeginning() {
	if (clips_.empty()) { Stop(); return; }
	StartClip(0);
}

void PlayerComboMotion::Stop() {
	phase_ = Phase::kIdle;
	index_ = -1;
	pendingNext_ = -1;
	time_ = 0.0f;
	blendTimer_ = 0.0f;
	queuedNext_ = false;
	hasHitThisClip_ = false;
}

// 次に再生すべきクリップ。無ければ -1。
int PlayerComboMotion::ResolveNextIndex() const {
	const int count = static_cast<int>(clips_.size());
	if (autoAdvance_) {
		// プレビュー: 常に次へ（末尾でループ指定ならば先頭へ）
		if (index_ + 1 < count) { return index_ + 1; }
		if (loop_ && count > 0) { return 0; }
		return -1;
	}
	// ゲーム: コンボ入力で次段が予約されているときのみ
	if (queuedNext_ && index_ + 1 < count) { return index_ + 1; }
	return -1;
}

// 現クリップ終端の姿勢を保持し、次クリップ開始姿勢へのブレンドを開始する
void PlayerComboMotion::BeginBlend(int nextIndex) {
	const float endT = clips_[index_].GetDuration();
	blendFrom_[0] = clips_[index_].SampleBody(endT);
	blendFrom_[1] = clips_[index_].SampleRArm(endT);
	blendFrom_[2] = clips_[index_].SampleLArm(endT);

	pendingNext_ = nextIndex;
	blendTimer_ = 0.0f;

	if (blendTime_ <= 0.0f) {
		// 補間時間ゼロなら即次クリップへ
		StartClip(nextIndex);
	} else {
		phase_ = Phase::kBlend;
	}
}

// コンボを継続しない場合、現クリップ終端姿勢から基準姿勢へ補間して戻す
void PlayerComboMotion::BeginReturn() {
	const float endT = clips_[index_].GetDuration();
	blendFrom_[0] = clips_[index_].SampleBody(endT);
	blendFrom_[1] = clips_[index_].SampleRArm(endT);
	blendFrom_[2] = clips_[index_].SampleLArm(endT);

	pendingNext_ = -1;
	blendTimer_ = 0.0f;

	if (blendTime_ <= 0.0f) {
		Stop(); // 補間時間ゼロなら即停止（ApplyVariablesが基準姿勢へ）
	} else {
		phase_ = Phase::kReturn;
	}
}

PlayerComboMotion::Result PlayerComboMotion::TryAdvance() {
	if (clips_.empty()) { return Result::kNone; }

	// 停止中／基準姿勢へ戻す補間中は、新たなコンボを先頭から開始する
	if (phase_ == Phase::kIdle || phase_ == Phase::kReturn) {
		StartClip(0);
		return Result::kAttacked;
	}

	// クリップ再生中のみ受付。コンボ受付ウィンドウが開いているか
	if (phase_ == Phase::kClip &&
		clips_[index_].IsComboWindowOpen(time_)) {
		if (index_ + 1 < static_cast<int>(clips_.size())) {
			queuedNext_ = true;
			return Result::kAttacked;
		}
		// 最終段で受付中の入力 → ラッシュへ
		return Result::kRush;
	}
	return Result::kNone;
}

void PlayerComboMotion::Update(float dt) {
	if (phase_ == Phase::kClip) {
		time_ += dt;
		const float duration = clips_[index_].GetDuration();
		if (time_ >= duration) {
			const int next = ResolveNextIndex();
			if (next >= 0) {
				BeginBlend(next);   // 次クリップへ繋ぐ
			} else {
				BeginReturn();      // 繋がない → 基準姿勢へ補間して戻す
			}
		}
	}
	else if (phase_ == Phase::kBlend) {
		blendTimer_ += dt;
		if (blendTimer_ >= blendTime_) {
			StartClip(pendingNext_);
		}
	}
	else if (phase_ == Phase::kReturn) {
		blendTimer_ += dt;
		if (blendTimer_ >= blendTime_) {
			Stop();
		}
	}
}

PartPose PlayerComboMotion::GetBodyPose() const {
	if (phase_ == Phase::kClip) { return clips_[index_].SampleBody(time_); }
	const float u = (blendTime_ > 0.0f) ? (blendTimer_ / blendTime_) : 1.0f;
	if (phase_ == Phase::kBlend) {
		return PlayerMotionClip::BlendPose(blendFrom_[0], clips_[pendingNext_].SampleBody(0.0f), u);
	}
	if (phase_ == Phase::kReturn) {
		return PlayerMotionClip::BlendPose(blendFrom_[0], PartPose{}, u); // 体は中立へ
	}
	return PartPose{};
}

PartPose PlayerComboMotion::GetRArmPose() const {
	if (phase_ == Phase::kClip) { return clips_[index_].SampleRArm(time_); }
	const float u = (blendTime_ > 0.0f) ? (blendTimer_ / blendTime_) : 1.0f;
	PartPose base; base.translate = kRArmBase_; // 右腕の基準姿勢
	if (phase_ == Phase::kBlend) {
		return PlayerMotionClip::BlendPose(blendFrom_[1], clips_[pendingNext_].SampleRArm(0.0f), u);
	}
	if (phase_ == Phase::kReturn) {
		return PlayerMotionClip::BlendPose(blendFrom_[1], base, u);
	}
	return base;
}

PartPose PlayerComboMotion::GetLArmPose() const {
	if (phase_ == Phase::kClip) { return clips_[index_].SampleLArm(time_); }
	const float u = (blendTime_ > 0.0f) ? (blendTimer_ / blendTime_) : 1.0f;
	PartPose base; base.translate = kLArmBase_; // 左腕の基準姿勢
	if (phase_ == Phase::kBlend) {
		return PlayerMotionClip::BlendPose(blendFrom_[2], clips_[pendingNext_].SampleLArm(0.0f), u);
	}
	if (phase_ == Phase::kReturn) {
		return PlayerMotionClip::BlendPose(blendFrom_[2], base, u);
	}
	return base;
}

bool PlayerComboMotion::IsHitActive() const {
	// ヒット判定はクリップ再生中のみ（ブレンド中は無し）
	if (phase_ != Phase::kClip || hasHitThisClip_) { return false; }
	return clips_[index_].IsInHitWindow(time_);
}

HitArm PlayerComboMotion::GetHitArm() const {
	if (phase_ != Phase::kClip) { return HitArm::kRight; }
	return clips_[index_].GetHitArm();
}

uint32_t PlayerComboMotion::GetDamage() const {
	if (phase_ != Phase::kClip) { return 0; }
	return clips_[index_].GetDamage();
}
