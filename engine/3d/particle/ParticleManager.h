#pragma once
#include "ParticleCommon.h"
#include "SrvManager.h"
#include "random"
#include "ViewProjection.h"
#include "WorldTransform.h"

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"

// パーティクル管理
class ParticleManager
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(SrvManager* srvManager);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(const ViewProjection& viewProjeciton);

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// パーティクルグループ生成
	/// </summary>
	/// <param name="name"></param>
	/// <param name="textureFilePath"></param>
	void CreateParticleGroup(const std::string name, const std::string& filename);

public:

	/// 各ステータス設定関数
	/// <returns></returns>
	void SetBillBorad(bool isBillBoard) { isBillboard = isBillBoard; }
	void SetRandomRotate(bool isRandomRotate) { isRandomRotate_ = isRandomRotate; }
	void SetAcceMultipy(bool isAcceMultipy) { isAcceMultipy_ = isAcceMultipy; }
	void SetRandomSize(bool isRandomSize) { isRandomSize_ = isRandomSize; }
	void SetAllRandomSize(bool isAllRandomSize) { isRandomAllSize_ = isAllRandomSize; }
	void SetSinMove(bool isSinMove) { isSinMove_ = isSinMove; }

private:

	/// <summary>
	/// 頂点データ作成
	/// </summary>
	void CreateVartexData(const std::string& filename);

private:

	struct ParticleForGPU {
		Matrix4x4 WVP;
		Matrix4x4 World;
		Vector4 color;
	};

	struct Particle {
		WorldTransform transform;
		Vector3 velocity;
		Vector3 Acce;
		Vector4 color; 
		float lifeTime;
		float currentTime;
		Vector3 startScale;
		Vector3 endScale;
		Vector3 startAcce;
		Vector3 endAcce;
		Vector3 startRote;
		Vector3 endRote;
		Vector3 rotateVelocity;
		float initialAlpha;
	};

	struct MaterialData
	{
		std::string textureFilePath;
	};

	struct ParticleGroup {
		MaterialData material;
		std::list<Particle> particles;
		uint32_t instancingSRVIndex = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource = nullptr;
		uint32_t instanceCount = 0;
		ParticleForGPU* instancingData = nullptr;
	};

	ParticleCommon* particleCommon = nullptr;
	SrvManager* srvManager_;

	// --- 頂点データ ---
	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
	VertexData* vertexData = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	// --- マテリアルデータ ---
	struct Material {
		Vector4 color;
		Matrix4x4 uvTransform;
		float padding[3];
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;
	Material* materialData = nullptr;


	struct ModelData
	{
		std::vector<VertexData> vertices;
		MaterialData material;
	};
	ModelData modelData;
	static std::unordered_map<std::string, ModelData> modelCache;
	std::unordered_map<std::string, ParticleGroup>particleGroups;

	// デルタタイム
	const float kDeltaTime = 1.0f / 60.0f;
	static const uint32_t kNumMaxInstance = 10000;

	std::random_device seedGenerator;
	std::mt19937 randomEngine;

	bool isBillboard = false;
	bool isRandomRotate_ = false;
	bool isAcceMultipy_ = false;
	bool isRandomSize_ = false;
	bool isRandomAllSize_ = false;
	bool isSinMove_ = false;

public:
	
	/// <summary>
	/// 指定した名前のパーティクルグループにパーティクルを発生させる
	/// </summary>
	std::list<Particle> Emit(const std::string name, const Vector3& position, uint32_t count, const Vector3& scale,
		const Vector3& velocityMin, const Vector3& velocityMax, float lifeTimeMin, float lifeTimeMax,
		const Vector3& particleStartScale, const Vector3& particleEndScale, const Vector3& startAcce, const Vector3& endAcce,
		const Vector3& startRote, const Vector3& endRote, bool isRandomColor, float alphaMin, float alphaMax,
		const Vector3& rotateVelocityMin, const Vector3& rotateVelocityMax,
		const Vector3& allScaleMax, const Vector3& allScaleMin,
		const float& scaleMin, const float& scaleMax, const Vector3& rotation);

private:
	/// <summary>
	/// .mtlファイルの読み取り
	/// </summary>
	/// <param name="directoryPath"></param>
	/// <param name="filename"></param>
	/// <returns></returns>
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

	/// <summary>
	///  .objファイルの読み取り
	/// </summary>
	/// <param name="directoryPath"></param>
	/// <param name="filename"></param>
	/// <returns></returns>
	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

	/// <summary>
	/// マテリアルデータ作成
	/// </summary>
	void CreateMaterial();

	Particle MakeNewParticle(std::mt19937& randomEngine,
		const Vector3& translate,
		const Vector3& rotation,
		const Vector3& scale,
		const Vector3& velocityMin, const Vector3& velocityMax,
		float lifeTimeMin, float lifeTimeMax, const Vector3& particleStartScale, const Vector3& particleEndScale,
		const Vector3& startAcce, const Vector3& endAcce, const Vector3& startRote, const Vector3& endRote
		, bool isRamdomColor, float alphaMin, float alphaMax, const Vector3& rotateVelocityMin, const Vector3& rotateVelocityMax,
		const Vector3& allScaleMax, const Vector3& allScaleMin,
		const float& scaleMin, const float& scaleMax);
};

