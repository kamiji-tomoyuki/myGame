#pragma once
#include "d3d12.h"
#include "string"
#include "vector"

#include "Model.h"
#include "ObjColor.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "light/LightGroup.h"
#include "animation/ModelAnimation.h"

#include "Matrix4x4.h"
#include "Vector2.h" 
#include "Vector3.h"
#include "Vector4.h"

class ModelCommon;
class Object3dCommon;

// オブジェクト3D
class Object3d
{
public: // メンバ関数

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(const std::string& filePath);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(const WorldTransform& worldTransform, const ViewProjection& viewProjection);

	/// <summary>
	/// アニメーションの更新処理
	/// </summary>
	void AnimationUpdate(bool roop);

	/// <summary>
	/// アニメーションの再生・停止
	/// </summary>
	/// <param name="anime"></param>
	void SetStopAnimation(bool anime) { modelAnimation_->SetIsAnimation(anime); }

	/// <summary>
	/// アニメーションのセット
	/// </summary>
	/// <param name="fileName"></param>
	void SetAnimation(const std::string& fileName);

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw(const WorldTransform& worldTransform, const ViewProjection& viewProjection, ObjColor* color = nullptr, bool Lighting = true);

	/// <summary>
	/// スケルトン描画処理
	/// </summary>
	void DrawSkeleton(const WorldTransform& worldTransform, const ViewProjection& viewProjection);

public:

	/// 各ステータス取得関数
	/// <returns></returns>
	const Vector3& GetPosition()const { return position; }
	const Vector3& GetRotation() const { return rotation; }
	const Vector3& GetSize() const { return size; }

	/// 各ステータス設定関数
	/// <returns></returns>
	void SetModel(Model* model) { this->model = model; }
	void SetPosition(const Vector3& position) { this->position = position; }
	void SetRotation(const Vector3& rotation) { this->rotation = rotation; }
	void SetSize(const Vector3& size) { this->size = size; }
	void SetColor(const Vector4& color) { materialData->color = color; }
	void SetModel(const std::string& filePath);
	
	/// <summary>
	/// 光沢度の設定
	/// </summary>
	/// <param name="shininess">マテリアルの光沢度</param>
	void SetShininess(float shininess = 20.0f);

private: // メンバ関数

	/// <summary>
	/// 座標変換行列データ作成
	/// </summary>
	void CreateTransformationMatrix();

	/// <summary>
	/// マテリアルデータ作成
	/// </summary>
	void CreateMaterial();

	Vector3 ExtractTranslation(const Matrix4x4& matrix) { return Vector3(matrix.m[3][0], matrix.m[3][1], matrix.m[3][2]); }

private:

	Object3dCommon* obj3dCommon = nullptr;

	// --- 座標変換行列データ ---
	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
		Matrix4x4 WorldInverseTranspose;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource;
	TransformationMatrix* transformationMatrixData = nullptr;

	// --- マテリアルデータ ---
	struct Material {
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
		float shininess;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;
	Material* materialData = nullptr;

	struct Transform {
		Vector3 scale;
		Vector3 rotate;
		Vector3 translate;
	};
	Transform transform;

	Model* model = nullptr;
	std::unique_ptr<ModelAnimation> modelAnimation_ = nullptr;
	ModelCommon* modelCommon = nullptr;
	LightGroup* lightGroup = nullptr;

	Vector3 position = { 0.0f,0.0f,0.0f };
	Vector3 rotation = { 0.0f,0.0f,0.0f };
	Vector3 size = { 1.0f,1.0f,1.0f };
};

