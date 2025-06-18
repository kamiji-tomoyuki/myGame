#pragma once
#include "ModelStructs.h"
#include "ModelCommon.h"
#include "SrvManager.h"
#include "animation/Animator.h"
#include "animation/Bone.h"
#include "animation/Skin.h"

#include "map"
#include "span"
#include "array"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include "Quaternion.h"

#include <unordered_set>

// モデル
class Model
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="modelCommon"></param>
	void Initialize(ModelCommon* modelCommon, const std::string& directorypath, const std::string& filename);

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

public:

	/// <summary>
	/// .gltfか判別
	/// </summary>
	bool IsGltf() { return isGltf; }

	/// 各ステータス取得関数
	/// <returns></returns>
	ModelData GetModelData() { return modelData; }
	bool CheckBone() const { return hasBone_; }

	/// 各ステータス設定関数
	/// <returns></returns>
	void SetSrv(SrvManager* srvManager) { srvManager_ = srvManager; }
	void SetAnimator(Animator* animator) { animator_ = animator; }
	void SetSkin(Skin* skin) { skin_ = skin; }
	void SetBone(Bone* bone) { bone_ = bone; }

private:

	/// <summary>
	/// 頂点データ作成
	/// </summary>
	void CreateVartexData();

	/// <summary>
    /// indexの作成
    /// </summary>
	void CreateIndexResource();

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
	static ModelData LoadModelFile(const std::string& directoryPath, const std::string& filename);

	/// <summary>
	/// ノード読み取り
	/// </summary>
	/// <param name="node"></param>
	/// <returns></returns>
	static Node ReadNode(aiNode* node);

private:
	ModelCommon* modelCommon_;

	ModelData modelData;
	SrvManager* srvManager_;

	// vertexResource
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
	VertexData* vertexData = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	// indexResource
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource = nullptr;
	uint32_t* indexData;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	std::string filename_;
	std::string directorypath_;

	Matrix4x4 localMatrix;

	static bool isGltf;
	Animator* animator_;
	Skin* skin_;
	Bone* bone_;

	bool hasBone_ = false;

	static std::unordered_set<std::string> jointNames;
};

