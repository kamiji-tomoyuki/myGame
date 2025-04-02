#include "ModelAnimation.h"

void ModelAnimation::Initialize(const std::string& directorypath, const std::string& filename)
{
	// --- その他引数の適応 ---
	directorypath_ = directorypath;
	filename_ = filename;

	// --- アニメーション関連生成・初期化 ---
	animator_ = std::make_unique<Animator>();
	bone_ = std::make_unique<Bone>();
	skin_ = std::make_unique<Skin>();
	animator_->Initialize(directorypath_, filename_);
	if (animator_->HaveAnimation()) {	
		// アニメーションがある時
		bone_->Initialize(modelData_);
		skin_->Initialize(bone_->GetSkeleton(), modelData_);
	}
}

void ModelAnimation::Update(bool roop)
{
	// --- アニメーションの更新処理　---
	if (animator_->HaveAnimation()) {
		animator_->Update(roop);
		bone_->Update(animator_->GetAnimation(), animator_->GetAnimationTime());
		skin_->Update(bone_->GetSkeleton());
	}
}