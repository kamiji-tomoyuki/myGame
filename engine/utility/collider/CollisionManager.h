#pragma once
#include "Collider.h"
#include "SceneManager.h"
#include "list"
#include "set"
#include "Object3d.h"

/// <summary>
/// 当たり判定管理クラス
/// </summary>
namespace Engine {
class CollisionManager {
private:
	// コライダー
	static std::list<Collider*> colliders_;

	// ペアごとの衝突状態管理
	using ColliderPair = std::pair<Collider*, Collider*>;
	static std::set<ColliderPair> previousCollidingPairs_; // 前フレームの衝突ペア
	static std::set<ColliderPair> currentCollidingPairs_;  // 現フレームの衝突ペア

	bool visible = true;
	bool sphereCollision = true;
	bool aabbCollision = true;
	bool obbCollision = true;

public:
	/// <summary>
	/// リセット
	/// </summary>
	static void Reset();

	/// <summary>
	/// コライダーの削除（破棄時にペアセットも掃除）
	/// </summary>
	static void RemoveCollider(Collider* collider);

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// ワールドトランスフォームの更新
	/// </summary>
	void UpdateWorldTransform();

	/// <summary>
	/// 描画
	/// </summary>
	/// <param name="viewProjection"></param>
	void Draw(const ViewProjection& viewProjection);

	void Update();

	/// <summary>
	/// 全ての当たり判定チェック
	/// </summary>
	void CheckAllCollisions();

	/// <summary>
	/// コライダーの登録
	/// </summary>
	static void AddCollider(Collider* collider);

private:
	// 調整項目の適用
	void ApplyGlobalVariables();

	// 2つのコライダー間の純粋な衝突判定
	bool CheckCollisionBetween(Collider* colliderA, Collider* colliderB);

	bool IsCollision(const AABB& aabb1, const AABB& aabb2);
	bool IsCollision(const OBB& obb1, const OBB& obb2);

	// 軸に対するOBBの投影範囲を計算する関数
	void projectOBB(const OBB& obb, const Vector3& axis, float& min, float& max);
	// 軸に投影するための関数
	bool testAxis(const Vector3& axis, const OBB& obb1, const OBB& obb2);
};
} // namespace Engine
