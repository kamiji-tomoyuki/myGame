#pragma once
#include "ModelStructs.h"

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

#include <Vector3.h>
#include <Quaternion.h>

// アニメーション管理
class Animator
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(const std::string& directorypath, const std::string& filename);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(bool roop);

public:

	/// <summary>
	/// アニメーションがいるか否か
	/// </summary>
	bool HaveAnimation() { return haveAnimation; }

	/// 各ステータス取得関数
	/// <returns></returns>
	Animation GetAnimation() { return animation_; }
	float GetAnimationTime() { return animationTime; }

	/// 各ステータス設定関数
	/// <returns></returns>
	void SetAnimationTime(float time) { animationTime = time; }
	void SetIsAnimation(bool isAnimation) { isAnimation_ = isAnimation; }

public:

	/// <summary>
	/// アニメーションファイル読み込み
	/// </summary>
	/// <param name="directoryPath"></param>
	/// <param name="filename"></param>
	/// <returns></returns>
	Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename);

	/// <summary>
	/// 値の計算(Vector3)
	/// </summary>
	/// <param name="keyframes"></param>
	/// <param name="time"></param>
	/// <returns></returns>
	static Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time);

	/// <summary>
	/// 値の計算(Quaternion)
	/// </summary>
	/// <param name="keyframes"></param>
	/// <param name="time"></param>
	/// <returns></returns>
	static Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time);

private:

	std::string filename_;
	std::string directorypath_;

	bool haveAnimation = false;

	Animation animation_;
	float animationTime = 0.0f;
	bool isRoop_;
	bool isAnimation_ = true;

	static std::unordered_map<std::string, Animation> animationCache;
};

