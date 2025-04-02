#include "Model.h"
#include "fstream"
#include "sstream"

#include "Frame.h"
#include "TextureManager.h"

#include "myMath.h"

bool Model::isGltf = false;
std::unordered_set<std::string> Model::jointNames = {};

void Model::Initialize(ModelCommon* modelCommon, const std::string& directorypath, const std::string& filename)
{
	// --- 引数で受け取りメンバ変数に記録 ---
	modelCommon_ = modelCommon;

	// --- パスを設定 ---
	directorypath_ = directorypath;
	filename_ = filename;
	srvManager_ = SrvManager::GetInstance();

	// --- モデル読み込み ---
	modelData = LoadModelFile(directorypath_, filename_);

	CreateVartexData();
	CreateIndexResource();

	// --- .objの参照しているテクスチャファイル読み込み ---
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	// 単位行列を書き込んでおく
	modelData.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);


}

void Model::Draw()
{
	D3D12_VERTEX_BUFFER_VIEW vbvs[2] = { vertexBufferView, {} };

	if (skin_) {
		vbvs[1] = skin_->GetSkinCluster().influenceBufferView;
	}

	if (animator_ && animator_->HaveAnimation()) {
		// アニメーションあり
		modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, skin_ ? 2 : 1, vbvs);
		if (skin_) {
			srvManager_->SetGraphicsRootDescriptorTable(6, skin_->GetSrvIndex());
		}
	}
	else {
		// アニメーションなし
		modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, vbvs);
	}

	modelCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&indexBufferView);
	srvManager_->SetGraphicsRootDescriptorTable(2, modelData.material.textureIndex);

	// --- 描画 ---
	modelCommon_->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(
		UINT(modelData.indices.size()), 1, 0, 0, 0);
}

void Model::CreateVartexData()
{
	vertexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
}

void Model::CreateIndexResource()
{
	indexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * modelData.indices.size());
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = UINT(sizeof(uint32_t) * modelData.indices.size());
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	std::memcpy(indexData, modelData.indices.data(), sizeof(uint32_t) * modelData.indices.size());
}

MaterialData Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename)
{
	MaterialData materialData;
	std::string line;

	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			materialData.textureFilePath = directoryPath + "../images/" + textureFilename;
		}
	}

	if (materialData.textureFilePath.empty()) {
		// テクスチャが設定されていない場合、"white1x1.png" を割り当てる
		materialData.textureFilePath = directoryPath + "/" + "white1x1.png";
	}

	return materialData;
}


ModelData Model::LoadModelFile(const std::string& directoryPath, const std::string& filename)
{
	ModelData modelData;

	// --- gltfか判定 ---
	isGltf = false;
	if (filename.size() >= 5 && filename.substr(filename.size() - 5) == ".gltf") {
		isGltf = true;
	}
	else if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".obj") {
		isGltf = false;
	}
	else {
		assert(false && "Unsupported file format"); // サポート外のフォーマットの場合にアサート
	}

	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);

	if (!scene || !scene->HasMeshes()) {
		// テクスチャが設定されていない場合、"white1x1.png" を割り当てる
		modelData.material.textureFilePath = directoryPath + "/" + "white1x1.png";
		return modelData;
	}

	// --- メッシュ処理 ---
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		// 法線、Texcoordがない場合エラーを出力
		assert(mesh->HasNormals());
		assert(mesh->HasTextureCoords(0));

		modelData.vertices.resize(mesh->mNumVertices);
		for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
			aiVector3D& position = mesh->mVertices[vertexIndex];
			aiVector3D& normal = mesh->mNormals[vertexIndex];
			aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];
			modelData.vertices[vertexIndex].position = { -position.x,position.y,position.z,1.0f };
			modelData.vertices[vertexIndex].normal = { -normal.x,normal.y,normal.z };
			modelData.vertices[vertexIndex].texcoord = { texcoord.x,texcoord.y };
		}
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3);
			for (uint32_t element = 0; element < face.mNumIndices; ++element) {
				uint32_t vertexIndex = face.mIndices[element];
				modelData.indices.push_back(vertexIndex);
			}
		}

		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
			aiBone* bone = mesh->mBones[boneIndex];
			std::string jointName = bone->mName.C_Str();

			assert(jointNames.find(jointName) == jointNames.end() && "Duplicate joint name detected!");
			jointNames.insert(jointName);

			aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
			aiVector3D scale, translate;
			aiQuaternion rotate;
			bindPoseMatrixAssimp.Decompose(scale, rotate, translate);

			Matrix4x4 bindPoseMatrix = MakeAffineMatrix(
				{ scale.x, scale.y, scale.z },
				{ rotate.x, -rotate.y, -rotate.z, rotate.w },
				{ -translate.x, translate.y, translate.z });

			JointWeightData& jointWeightData = modelData.skinClusterData[jointName];
			jointWeightData.inverseBindPoseMatrix = Inverse(bindPoseMatrix);

			for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
				jointWeightData.vertexWeights.push_back({
					bone->mWeights[weightIndex].mWeight,
					bone->mWeights[weightIndex].mVertexId
					});
			}
		}
	}
	// クリア
	jointNames.clear();

	// --- マテリアル処理 ---
	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex) {
		aiMaterial* material = scene->mMaterials[materialIndex];
		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
			modelData.material.textureFilePath = directoryPath + textureFilePath.C_Str();
		}
	}
	if (modelData.material.textureFilePath.empty()) {
		// テクスチャが設定されていない場合、"white1x1.png" を割り当てる
		modelData.material.textureFilePath = directoryPath + "/" + "white1x1.png";
	}
	modelData.rootNode = ReadNode(scene->mRootNode);
	return modelData;
}

Node Model::ReadNode(aiNode* node)
{
	Node result;

	aiVector3D scale, translate;
	aiQuaternion rotate;
	node->mTransformation.Decompose(scale, rotate, translate);
	result.transform.scale = { scale.x,scale.y,scale.z };
	result.transform.rotate = { rotate.x,-rotate.y,-rotate.z,rotate.w };
	result.transform.translate = { -translate.x,translate.y,translate.z };

	result.localMatrix = MakeAffineMatrix(result.transform.scale, result.transform.rotate, result.transform.translate);

	result.name = node->mName.C_Str();
	result.children.resize(node->mNumChildren);
	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
		result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
	}
	return result;
}