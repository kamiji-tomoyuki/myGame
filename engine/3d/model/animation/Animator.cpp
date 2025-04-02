#include "Animator.h"
#include <cassert>

#include <myMath.h>
#include <Frame.h>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>

std::unordered_map<std::string, Animation> Animator::animationCache;

void Animator::Initialize(const std::string& directorypath, const std::string& filename)
{
	haveAnimation = false;

	// --- 引数の適応 ---
	directorypath_ = directorypath;
	filename_ = filename;

	// --- ファイル読み込み ---
	animation_ = LoadAnimationFile(directorypath_, filename_);
}

void Animator::Update(bool roop)
{
	if (isAnimation_) {
		if (roop) {
			// --- ループ時の処理 ---
			// アニメーション時間を進め、超えたら最初に戻る
			animationTime += Frame::DeltaTime();
			animationTime = std::fmod(animationTime, animation_.duration);
		} else {
			// --- 非ループ時の処理 ---
			// アニメーションが終了するまで進行
			if (animationTime < animation_.duration) {
				animationTime += Frame::DeltaTime();
				// durationを超えたら停止
				if (animationTime > animation_.duration) {
					animationTime = animation_.duration;
					isAnimation_ = false;
				}
			}
		}
	}
}

Animation Animator::LoadAnimationFile(const std::string& directoryPath, const std::string& filename)
{
	// --- パスの生成・チェック ---
	std::string filePath = directoryPath + "/" + filename;
	Animation animation;
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);
	if (!scene || scene->mNumAnimations == 0) {
		// アニメーションなし
		haveAnimation = false;
		return animation;
	}

	haveAnimation = true;

	auto it = animationCache.find(filePath);
	if (it != animationCache.end()) {
		return it->second;  // すでに読み込み済みの場合パスから返す
	}

	aiAnimation* animationAssimp = scene->mAnimations[0];
	animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond);

	// --- ノードアニメーション読み込み ---
	for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
		aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
		NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];

		// transrate
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; ++keyIndex) {
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
			KeyframeVector3 keyframe;
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
			keyframe.value = { -keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };
			nodeAnimation.translate.push_back(keyframe);
		}

		// Rotate
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumRotationKeys; ++keyIndex) {
			aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
			KeyframeQuaternion keyframe;
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
			keyframe.value = { keyAssimp.mValue.x, -keyAssimp.mValue.y, -keyAssimp.mValue.z, keyAssimp.mValue.w };
			nodeAnimation.rotate.push_back(keyframe);
		}

		// Scale
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; ++keyIndex) {
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
			KeyframeVector3 keyframe;
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
			keyframe.value = { keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };
			nodeAnimation.scale.push_back(keyframe);
		}
	}

	// キャッシュに登録
	animationCache[filePath] = animation;
	return animation;
}

Vector3 Animator::CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time)
{
	// --- Vector3 ---
	assert(!keyframes.empty()); 
	if (keyframes.size() == 1 || time <= keyframes[0].time) {
		return keyframes[0].value;
	}

	for (size_t index = 0; index < keyframes.size() - 1; ++index) {
		size_t nextIndex = index + 1;
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
			float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
			return Lerp(keyframes[index].value, keyframes[nextIndex].value, t);
		}
	}
	return (*keyframes.rbegin()).value;
}

Quaternion Animator::CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time)
{
	// --- Quaternion ---
	assert(!keyframes.empty());
	if (keyframes.size() == 1 || time <= keyframes[0].time) {
		return keyframes[0].value;
	}

	for (size_t index = 0; index < keyframes.size() - 1; ++index) {
		size_t nextIndex = index + 1;
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
			float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
			return Slerp(keyframes[index].value, keyframes[nextIndex].value, t);
		}
	}
	return (*keyframes.rbegin()).value;
}