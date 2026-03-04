#define NOMINMAX
#include "CollisionManager.h"
#include "GlobalVariables.h"
#include "Object3dCommon.h"
#include "myMath.h"

// 静的メンバの定義
std::list<Collider*>                          CollisionManager::colliders_;
std::set<CollisionManager::ColliderPair>      CollisionManager::previousCollidingPairs_;
std::set<CollisionManager::ColliderPair>      CollisionManager::currentCollidingPairs_;

void CollisionManager::Reset() {
	colliders_.clear();
	previousCollidingPairs_.clear();
	currentCollidingPairs_.clear();
}

void CollisionManager::RemoveCollider(Collider* collider) {
	// colliders_ から除去
	auto it = std::find(colliders_.begin(), colliders_.end(), collider);
	if (it != colliders_.end()) {
		colliders_.erase(it);
	}

	// 破棄されるコライダーを含むペアを両方のセットから除去
	// → ダングリングポインタによるアクセス違反を防ぐ
	auto removePairsContaining = [&](std::set<ColliderPair>& pairs) {
		for (auto it = pairs.begin(); it != pairs.end(); ) {
			if (it->first == collider || it->second == collider) {
				it = pairs.erase(it);
			}
			else {
				++it;
			}
		}
		};

	removePairsContaining(previousCollidingPairs_);
	removePairsContaining(currentCollidingPairs_);
}

void CollisionManager::Initialize() {
	const char* groupName = "Collider";
	GlobalVariables::GetInstance()->CreateGroup(groupName);

	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	globalVariables->AddItem(groupName, "visible", visible);
	globalVariables->AddItem(groupName, "sphereCollision", sphereCollision);
	globalVariables->AddItem(groupName, "aabbCollision", aabbCollision);
	globalVariables->AddItem(groupName, "obbCollision", obbCollision);
}

void CollisionManager::UpdateWorldTransform() {
	ApplyGlobalVariables();
	for (Collider* collider : colliders_) {
		collider->UpdateWorldTransform();
	}
}

void CollisionManager::Draw(const ViewProjection& viewProjection) {
	if (!visible) {
		return;
	}
	for (Collider* collider : colliders_) {
		if (!collider->IsCollisionEnabled()) {
			continue;
		}
		if (sphereCollision) {
			collider->DrawSphere(viewProjection);
		}
		if (aabbCollision) {
			collider->DrawAABB(viewProjection);
		}
		if (obbCollision) {
			collider->DrawOBB(viewProjection);
		}
	}
}

void CollisionManager::Update()
{
	CheckAllCollisions();
	UpdateWorldTransform();
}

bool CollisionManager::CheckCollisionBetween(Collider* colliderA, Collider* colliderB) {
	// コリジョンが無効化されている場合はスキップ
	if (!colliderA->IsCollisionEnabled() || !colliderB->IsCollisionEnabled()) {
		return false;
	}

	// 球の衝突チェック
	if (sphereCollision) {
		Vector3 posA = colliderA->GetCenter();
		Vector3 posB = colliderB->GetCenter();
		float distance = (posA - posB).Length();
		if (distance <= colliderA->GetRadius() + colliderB->GetRadius()) {
			return true;
		}
	}

	// AABBの衝突チェック
	if (aabbCollision) {
		if (IsCollision(colliderA->GetAABB(), colliderB->GetAABB())) {
			return true;
		}
	}

	// OBB同士の衝突チェック
	if (obbCollision) {
		if (IsCollision(colliderA->GetOBB(), colliderB->GetOBB())) {
			return true;
		}
	}

	return false;
}

void CollisionManager::CheckAllCollisions() {
	// 現フレームの衝突ペアをクリア
	currentCollidingPairs_.clear();

	// 全ペアの衝突判定（純粋な判定のみ、コールバックはまだ呼ばない）
	auto itrA = colliders_.begin();
	for (; itrA != colliders_.end(); ++itrA) {
		Collider* colliderA = *itrA;

		auto itrB = std::next(itrA);
		for (; itrB != colliders_.end(); ++itrB) {
			Collider* colliderB = *itrB;

			if (CheckCollisionBetween(colliderA, colliderB)) {
				// ポインタの大小でペアを正規化（順序を統一し重複を防ぐ）
				ColliderPair pair = (colliderA < colliderB)
					? ColliderPair(colliderA, colliderB)
					: ColliderPair(colliderB, colliderA);
				currentCollidingPairs_.insert(pair);
			}
		}
	}

	// 現フレームで衝突しているペアのコールバックを呼ぶ
	for (const auto& pair : currentCollidingPairs_) {
		if (previousCollidingPairs_.find(pair) == previousCollidingPairs_.end()) {
			// 前フレームになかった → 衝突開始
			pair.first->OnCollisionEnter(pair.second);
			pair.second->OnCollisionEnter(pair.first);
		}
		else {
			// 前フレームでも衝突していた → 衝突継続
			pair.first->OnCollision(pair.second);
			pair.second->OnCollision(pair.first);
		}
		pair.first->SetHitColor();
		pair.second->SetHitColor();
	}

	// 前フレームで衝突していたが、現フレームにないペア → 衝突終了
	for (const auto& pair : previousCollidingPairs_) {
		if (currentCollidingPairs_.find(pair) == currentCollidingPairs_.end()) {
			pair.first->OnCollisionOut(pair.second);
			pair.second->OnCollisionOut(pair.first);
			pair.first->SetDefaultColor();
			pair.second->SetDefaultColor();
		}
	}

	// 前フレームの状態を現フレームで更新
	previousCollidingPairs_ = currentCollidingPairs_;
}

void CollisionManager::AddCollider(Collider* collider)
{
	colliders_.push_back(collider);
}

void CollisionManager::ApplyGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Collider";
	visible = globalVariables->GetBoolValue(groupName, "visible");
	sphereCollision = globalVariables->GetBoolValue(groupName, "sphereCollision");
	aabbCollision = globalVariables->GetBoolValue(groupName, "aabbCollision");
	obbCollision = globalVariables->GetBoolValue(groupName, "obbCollision");
}

bool CollisionManager::IsCollision(const AABB& aabb1, const AABB& aabb2) {
	if ((aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) &&
		(aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) &&
		(aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z)) {
		return true;
	}
	return false;
}

bool CollisionManager::IsCollision(const OBB& obb1, const OBB& obb2) {
	Vector3 axes[15] = {
		obb1.orientations[0],
		obb1.orientations[1],
		obb1.orientations[2],
		obb2.orientations[0],
		obb2.orientations[1],
		obb2.orientations[2],
		obb1.orientations[0].Cross(obb2.orientations[0]),
		obb1.orientations[0].Cross(obb2.orientations[1]),
		obb1.orientations[0].Cross(obb2.orientations[2]),
		obb1.orientations[1].Cross(obb2.orientations[0]),
		obb1.orientations[1].Cross(obb2.orientations[1]),
		obb1.orientations[1].Cross(obb2.orientations[2]),
		obb1.orientations[2].Cross(obb2.orientations[0]),
		obb1.orientations[2].Cross(obb2.orientations[1]),
		obb1.orientations[2].Cross(obb2.orientations[2]),
	};

	for (const Vector3& axis : axes) {
		if (axis.Length() > 0.0001f && !testAxis(axis.Normalize(), obb1, obb2)) {
			return false;
		}
	}

	return true;
}

void CollisionManager::projectOBB(const OBB& obb, const Vector3& axis, float& min, float& max) {
	float centerProjection = obb.center.Dot(axis);
	float radius = std::abs(obb.orientations[0].Dot(axis)) * obb.size.x +
		std::abs(obb.orientations[1].Dot(axis)) * obb.size.y +
		std::abs(obb.orientations[2].Dot(axis)) * obb.size.z;

	min = centerProjection - radius;
	max = centerProjection + radius;
}

bool CollisionManager::testAxis(const Vector3& axis, const OBB& obb1, const OBB& obb2) {
	float min1, max1, min2, max2;
	projectOBB(obb1, axis, min1, max1);
	projectOBB(obb2, axis, min2, max2);

	float sumSpan = (max1 - min1) + (max2 - min2);
	float longSpan = std::max(max1, max2) - std::min(min1, min2);

	return sumSpan >= longSpan;
}