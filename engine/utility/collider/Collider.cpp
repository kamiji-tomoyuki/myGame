#include "Collider.h"
#include"CollisionManager.h"
#include <line/DrawLine3D.h>

int Collider::counter = -1;  // 初期値を-1に変更

Collider::Collider() {
	Cubewt_.Initialize();
	AABBwt_.Initialize();
	OBBwt_.Initialize();
	CollisionManager::AddCollider(this);
	sphere_ = std::make_unique<Object3d>();
	sphere_->Initialize("debug/Collider.obj");
	AABB_ = std::make_unique<Object3d>();
	AABB_->Initialize("debug/AABB.obj");
	OBB_ = std::make_unique<Object3d>();
	OBB_->Initialize("debug/AABB.obj");
	variables_ = GlobalVariables::GetInstance();

	// 派生クラス名を取得して整形
	std::string className = typeid(*this).name();
	size_t pos = className.find("Collider");
	if (pos != std::string::npos) {
		className.replace(pos, 8, ""); // "Collider"部分を削除
	}

	counter++;
	groupName = className + " Collider" + std::to_string(counter);

	// 初期化
	SphereOffset = { 0.0f, 0.0f, 0.0f };
	AABBOffset.min = { 0.0f, 0.0f, 0.0f };
	AABBOffset.max = { 0.0f, 0.0f, 0.0f };
	OBBOffset.center = { 0.0f,0.0f,0.0f };
	OBBOffset.size = { 0.0f,0.0f,0.0f };

	// グループがまだ存在しない場合のみ作成
	if (!variables_->GroupExists(groupName)) {
		variables_->CreateGroup(groupName);
		variables_->AddItem(groupName, "Sphere Translation", SphereOffset);
		variables_->AddItem(groupName, "AABB Min", AABBOffset.min);
		variables_->AddItem(groupName, "AABB Max", AABBOffset.max);
		variables_->AddItem(groupName, "OBB center", OBBOffset.center);
		variables_->AddItem(groupName, "OBB size", OBBOffset.size);
	}
}

Collider::~Collider()
{
	CollisionManager::RemoveCollider(this);
	counter--;  // カウンターをデクリメント
}

void Collider::Initialize() {
}

void Collider::UpdateWorldTransform() {
	ApplyVariables();

	// 球用のワールドトランスフォームを更新
	Cubewt_.translation_ = GetCenterPosition() + SphereOffset;
	Cubewt_.scale_ = { radius_, radius_, radius_ };
	Cubewt_.UpdateMatrix();

	// AABBの現在の最小点と最大点を取得
	aabb.min = GetCenterPosition() - scale_;
	aabb.max = GetCenterPosition() + scale_;
	aabb.min = aabb.min + AABBOffset.min;
	aabb.max = aabb.max + AABBOffset.max;

	// AABBの最小点と最大点からスケールと中心位置を計算
	aabbCenter = (aabb.min + aabb.max) * 0.5f;
	aabbScale = (aabb.max - aabb.min) * 0.5f;

	// AABB用ワールドトランスフォームの更新
	AABBwt_.translation_ = aabbCenter;
	AABBwt_.scale_ = aabbScale;
	AABBwt_.UpdateMatrix();

	obb.center = GetCenterPosition();
	obb.center = obb.center + OBBOffset.center;
	MakeOBBOrientations(obb, GetCenterRotation());
	obb.size = { 1.0f,1.0f,1.0f };
	obb.size = obb.size + OBBOffset.size;

	OBBwt_.translation_ = obb.center;
	OBBwt_.rotation_ = GetCenterRotation();
	OBBwt_.scale_ = obb.size;
	OBBwt_.UpdateMatrix();
}

void Collider::DrawSphere(const ViewProjection& viewProjection) {
	const uint32_t kSubdivision = 10;                                        // 分割数
	const float kLonEvery = 2.0f * std::numbers::pi_v<float> / kSubdivision; // 経度分割1つ分の角度
	const float kLatEvery = std::numbers::pi_v<float> / kSubdivision;        // 緯度分割1つ分の角度

	// 緯度の方向に分割　-π/2 ～ π/2
	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex; // 現在の緯度

		// 経度の方向に分割 0 ～ 2π
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			float lon = lonIndex * kLonEvery; // 現在の経度

			// 現在の点を求める
			Vector3 start = {
				Cubewt_.translation_.x + Cubewt_.scale_.x * std::cosf(lat) * std::cosf(lon),
				Cubewt_.translation_.y + Cubewt_.scale_.y * std::sinf(lat),
				Cubewt_.translation_.z + Cubewt_.scale_.z * std::cosf(lat) * std::sinf(lon)
			};

			// 次の点を求める（経度方向）
			Vector3 end1 = {
				Cubewt_.translation_.x + Cubewt_.scale_.x * std::cosf(lat) * std::cosf(lon + kLonEvery),
				Cubewt_.translation_.y + Cubewt_.scale_.y * std::sinf(lat),
				Cubewt_.translation_.z + Cubewt_.scale_.z * std::cosf(lat) * std::sinf(lon + kLonEvery),
			};

			// 次の点を求める（緯度方向）
			Vector3 end2 = {
				Cubewt_.translation_.x + Cubewt_.scale_.x * std::cosf(lat + kLatEvery) * std::cosf(lon),
				Cubewt_.translation_.y + Cubewt_.scale_.y * std::sinf(lat + kLatEvery),
				Cubewt_.translation_.z + Cubewt_.scale_.z * std::cosf(lat + kLatEvery) * std::sinf(lon),
			};

			// 線を描画（経度方向）
			DrawLine3D::GetInstance()->SetPoints(start, end1, color_);
			// 線を描画（緯度方向）
			DrawLine3D::GetInstance()->SetPoints(start, end2, color_);
		}
	}
}


void Collider::DrawAABB(const ViewProjection& viewProjection)
{
	// AABBの頂点リスト
	std::array<Vector3, 8> vertices = {
		aabb.min,
		{ aabb.max.x, aabb.min.y, aabb.min.z },
		{ aabb.min.x, aabb.max.y, aabb.min.z },
		{ aabb.max.x, aabb.max.y, aabb.min.z },
		{ aabb.min.x, aabb.min.y, aabb.max.z },
		{ aabb.max.x, aabb.min.y, aabb.max.z },
		{ aabb.min.x, aabb.max.y, aabb.max.z },
		{ aabb.max.x, aabb.max.y, aabb.max.z }
	};

	// エッジ接続リスト
	const std::array<std::pair<int, int>, 12> edges = {
		std::make_pair(0, 1), std::make_pair(1, 3), std::make_pair(3, 2), std::make_pair(2, 0), // 前面
		std::make_pair(4, 5), std::make_pair(5, 7), std::make_pair(7, 6), std::make_pair(6, 4), // 背面
		std::make_pair(0, 4), std::make_pair(1, 5), std::make_pair(2, 6), std::make_pair(3, 7)  // 側面
	};

	// 線を描画
	for (const auto& edge : edges) {
		DrawLine3D::GetInstance()->SetPoints(vertices[edge.first], vertices[edge.second], color_);
	}
}

void Collider::DrawOBB(const ViewProjection& viewProjection) {
	// OBBの8つの頂点を計算
	std::array<Vector3, 8> vertices;

	Vector3 halfSize = obb.size;
	for (int i = 0; i < 8; i++) {
		Vector3 offset = Vector3(
			(i & 1) ? halfSize.x : -halfSize.x,
			(i & 2) ? halfSize.y : -halfSize.y,
			(i & 4) ? halfSize.z : -halfSize.z
		);

		vertices[i] = obb.center +
			obb.orientations[0] * offset.x +
			obb.orientations[1] * offset.y +
			obb.orientations[2] * offset.z;
	}

	// エッジ接続リスト
	const std::array<std::pair<int, int>, 12> edges = {
		std::make_pair(0, 1), std::make_pair(1, 3), std::make_pair(3, 2), std::make_pair(2, 0), // 前面
		std::make_pair(4, 5), std::make_pair(5, 7), std::make_pair(7, 6), std::make_pair(6, 4), // 背面
		std::make_pair(0, 4), std::make_pair(1, 5), std::make_pair(2, 6), std::make_pair(3, 7)  // 側面
	};

	// 線を描画
	for (const auto& edge : edges) {
		DrawLine3D::GetInstance()->SetPoints(vertices[edge.first], vertices[edge.second], color_);
	}
}


void Collider::ApplyVariables()
{
	SphereOffset = variables_->GetVector3Value(groupName, "Sphere Translation");
	AABBOffset.min = variables_->GetVector3Value(groupName, "AABB Min");
	AABBOffset.max = variables_->GetVector3Value(groupName, "AABB Max");
	OBBOffset.center = variables_->GetVector3Value(groupName, "OBB center");
	OBBOffset.size = variables_->GetVector3Value(groupName, "OBB size");
}

void Collider::MakeOBBOrientations(OBB& obb, const Vector3& rotate) {
	Matrix4x4 rotateMatrix = MakeRotateXMatrix(rotate.x) * (MakeRotateYMatrix(rotate.y) * MakeRotateZMatrix(rotate.z));
	obb.orientations[0].x = rotateMatrix.m[0][0];
	obb.orientations[0].y = rotateMatrix.m[0][1];
	obb.orientations[0].z = rotateMatrix.m[0][2];

	obb.orientations[1].x = rotateMatrix.m[1][0];
	obb.orientations[1].y = rotateMatrix.m[1][1];
	obb.orientations[1].z = rotateMatrix.m[1][2];

	obb.orientations[2].x = rotateMatrix.m[2][0];
	obb.orientations[2].y = rotateMatrix.m[2][1];
	obb.orientations[2].z = rotateMatrix.m[2][2];
}
