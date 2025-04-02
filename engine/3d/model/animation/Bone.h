#pragma once
#include "ModelStructs.h"

// ボーン
class Bone
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(ModelData modelData);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(const Animation& animation, float animtaionTime);

public:

	/// 各ステータス取得関数
	/// <returns></returns>
	Skeleton GetSkeleton() { return skeleton_; }
	
	/// 各ステータス設定関数
	/// <returns></returns>
	void SetSkeleton(Skeleton& skeleton) { skeleton_ = skeleton; }

private:

	/// <summary>
	/// Joint作成
	/// </summary>
	/// <param name="node"></param>
	/// <param name="parent"></param>
	/// <param name="joints"></param>
	/// <returns></returns>
	int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);

	/// <summary>
	/// 骨作成
	/// </summary>
	/// <param name="rootNode"></param>
	/// <returns></returns>
	Skeleton CreateSkeleton(const Node& rootNode);


	/// <summary>
	/// アニメーションの適応
	/// </summary>
	/// <param name="skeleton"></param>
	/// <param name="animation"></param>
	/// <param name="animtionTime"></param>
	void ApplyAnimation(const Animation& animation, float animationTime);

private:

	Skeleton skeleton_;
};

