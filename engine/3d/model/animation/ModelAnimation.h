#pragma once
#include <memory>

#include "Animator.h"
#include "Bone.h"
#include "Skin.h"

// アニメーションモデル
class ModelAnimation
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

	/// 各ステータス取得関数
	/// <returns></returns>
	Skeleton GetSkeletonData() { return bone_->GetSkeleton(); }
	Animator* GetAnimator() { return animator_.get(); }
	Bone* GetBone() { return bone_.get(); }
	Skin* GetSkin() { return skin_.get(); }

	/// 各ステータス設定関数
	/// <returns></returns>
	void SetModelData(ModelData modelData) { modelData_ = modelData; }
	void SetIsAnimation(bool anime) { animator_->SetIsAnimation(anime); }

private:

	std::unique_ptr<Animator> animator_;
	std::unique_ptr<Bone> bone_;
	std::unique_ptr<Skin> skin_;

	std::string directorypath_;
	std::string filename_;

	ModelData modelData_;
};

