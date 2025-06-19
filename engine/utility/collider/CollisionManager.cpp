#define NOMINMAX
#include "CollisionManager.h"
#include "myMath.h"
#include "GlobalVariables.h"
#include"Object3dCommon.h"
std::list<Collider*>  CollisionManager::colliders_;
void CollisionManager::Reset() {
	// リストを空っぽにする
	colliders_.clear();
}

// Colliderを削除する
void CollisionManager::RemoveCollider(Collider* collider) {
	// colliderが存在するか確認し、存在すれば削除
	auto it = std::find(colliders_.begin(), colliders_.end(), collider);
	if (it != colliders_.end()) {
		colliders_.erase(it);
	}
}

void CollisionManager::Initialize() {
	const char* groupName = "Collider";
	// グループを追加
	GlobalVariables::GetInstance()->CreateGroup(groupName);

	GlobalVariables* globalVariables = GlobalVariables::GetInstance();

	// グローバル変数を追加
	globalVariables->AddItem(groupName, "visible", visible);
	globalVariables->AddItem(groupName, "sphereCollision", sphereCollision);
	globalVariables->AddItem(groupName, "aabbCollision", aabbCollision);
	globalVariables->AddItem(groupName, "obbCollision", obbCollision);
}


void CollisionManager::UpdateWorldTransform() {
	ApplyGlobalVariables();
	//// 非表示なら抜ける
	//if (!visible) {
	//	return;
	//}
	// 全てのコライダーについて
	for (Collider* collider : colliders_) {
		// 更新
		collider->UpdateWorldTransform();
	}
}

void CollisionManager::Draw(const ViewProjection& viewProjection) {
	// 非表示なら抜ける
	if (!visible) {
		return;
	}
	// 全てのコライダーについて
	for (Collider* collider : colliders_) {
		if (!collider->IsCollisionEnabled()) {
			continue;
		}
		if (sphereCollision) {
			// 描画
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

void CollisionManager::CheckCollisionPair(Collider* colliderA, Collider* colliderB) {
	isCollidingNow = false;
	// コリジョンが無効化されている場合はチェックをスキップ
	if (!colliderA->IsCollisionEnabled() || !colliderB->IsCollisionEnabled()) {
		return;
	}

	// 球の衝突チェック
	if (sphereCollision) {
		Vector3 posA = colliderA->GetCenter();
		Vector3 posB = colliderB->GetCenter();
		float distance = (posA - posB).Length();
		isCollidingNow = (distance <= colliderA->GetRadius() + colliderB->GetRadius());
	}

	// AABBの衝突チェック
	if (aabbCollision && !isCollidingNow) {
		isCollidingNow = IsCollision(colliderA->GetAABB(), colliderB->GetAABB());
	}

	// OBB同士の衝突チェック
	if (obbCollision && !isCollidingNow) {
		OBB obbA = colliderA->GetOBB();
		OBB obbB = colliderB->GetOBB();
		isCollidingNow = IsCollision(obbA, obbB);
	}

	// 衝突状態の変化に応じたコールバックの呼び出し
	if (isCollidingNow) {
		if (!colliderA->WasColliding() && !colliderB->WasColliding()) {
			colliderA->OnCollisionEnter(colliderB);
			colliderB->OnCollisionEnter(colliderA);
		}
		else {
			colliderA->OnCollision(colliderB);
			colliderB->OnCollision(colliderA);
		}
		colliderA->SetHitColor();
		colliderB->SetHitColor();
	}
	else {
		if (colliderA->WasColliding() || colliderB->WasColliding()) {
			colliderA->OnCollisionOut(colliderB);
			colliderB->OnCollisionOut(colliderA);
		}
		colliderA->SetDefaultColor();
		colliderB->SetDefaultColor();
	}

	colliderA->SetIsColliding(isCollidingNow);
	colliderB->SetIsColliding(isCollidingNow);
}

void CollisionManager::CheckAllCollisions() {
	// リスト内のペアを総当たり
	std::list<Collider*>::iterator itrA = colliders_.begin();
	for (; itrA != colliders_.end(); ++itrA) {
		Collider* colliderA = *itrA;

		// イテレータBはイテレータAの次の要素から回す（重複判定を回避）
		std::list<Collider*>::iterator itrB = itrA;
		itrB++;

		for (; itrB != colliders_.end(); ++itrB) {
			Collider* colliderB = *itrB;

			// ベアの当たり判定
			CheckCollisionPair(colliderA, colliderB);
		}
	}

}

void CollisionManager::AddCollider(Collider* collider)
{
	// コライダーをリストに追加する
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
	// 軸ごとに判定
	if ((aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) &&
		(aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) &&
		(aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z)) {
		return true;
	}
	return false;
}

bool CollisionManager::IsCollision(const OBB& obb1, const OBB& obb2) {
	// 15個の軸を準備
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

	// 各軸に対してSATを使って衝突判定を行う
	for (const Vector3& axis : axes) {
		if (axis.Length() > 0.0001f && !testAxis(axis.Normalize(), obb1, obb2)) {
			return false;
		}
	}


	return true;  // 全ての軸で衝突している場合はtrue
}

// 軸に対するOBBの投影範囲を計算する関数
void CollisionManager::projectOBB(const OBB& obb, const Vector3& axis, float& min, float& max) {
	float centerProjection = obb.center.Dot(axis);
	float radius = std::abs(obb.orientations[0].Dot(axis)) * obb.size.x + std::abs(obb.orientations[1].Dot(axis)) * obb.size.y + std::abs(obb.orientations[2].Dot(axis)) * obb.size.z;

	min = centerProjection - radius;
	max = centerProjection + radius;
}

// 軸に投影するための関数
bool CollisionManager::testAxis(const Vector3& axis, const OBB& obb1, const OBB& obb2) {
	float min1, max1, min2, max2;
	projectOBB(obb1, axis, min1, max1);
	projectOBB(obb2, axis, min2, max2);

	float sumSpan = (max1 - min1) + (max2 - min2);
	float longSpan = std::max(max1, max2) - std::min(min1, min2);

	return sumSpan >= longSpan;
}