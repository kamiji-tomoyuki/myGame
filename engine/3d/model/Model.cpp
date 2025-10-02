#include "Model.h"
#include "fstream"
#include "sstream"

#include "Frame.h"
#include "TextureManager.h"

#include "myMath.h"

bool Model::isGltf = false;
std::unordered_set<std::string> Model::jointNames = {};

void Model::Initialize(ModelCommon* modelCommon, const std::string& directorypath, const std::string& filename) {
    // --- 引数で受け取りメンバ変数に記録 ---
    modelCommon_ = modelCommon;
    directorypath_ = directorypath;
    filename_ = filename;
    srvManager_ = SrvManager::GetInstance();

    modelData = LoadModelFile(directorypath_, filename_);

    CreateVartexData();
    CreateIndexResource();

    // スキニングアニメーションか判別
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(directorypath_ + filename_, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    hasBone_ = false;
    if (scene) {
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            if (scene->mMeshes[i]->HasBones()) {
                hasBone_ = true;
                break;
            }
        }
    }

    // テクスチャ読み込み - 各メッシュのマテリアルに対して
    for (auto& mesh : modelData.meshes) {
        TextureManager::GetInstance()->LoadModelTexture(mesh.material.textureFilePath);
        mesh.material.textureIndex = TextureManager::GetInstance()->GetModelTextureIndexByFilePath(mesh.material.textureFilePath);
    }

    // アニメーター、ボーン、スキンの初期化は後で行う
    animator_ = nullptr;
    bone_ = nullptr;
    skin_ = nullptr;
}

void Model::Draw() {
    // 各メッシュを描画
    for (size_t meshIndex = 0; meshIndex < meshResources_.size(); ++meshIndex) {
        const auto& meshResource = meshResources_[meshIndex];
        const auto& meshData = modelData.meshes[meshIndex];

        if (!animator_ || !animator_->HaveAnimation()) {
            // アニメーションなし - 通常の頂点バッファのみ使用
            modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &meshResource.vertexBufferView);
            modelCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&meshResource.indexBufferView);
            srvManager_->SetGraphicsRootDescriptorTable(2, meshData.material.textureIndex);
            srvManager_->SetGraphicsRootDescriptorTable(6, environmentSrvIndex);
        }
        else if (!CheckBone()) {
            // アニメーションあり - 通常の頂点バッファのみ使用
            modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &meshResource.vertexBufferView);
            modelCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&meshResource.indexBufferView);
            srvManager_->SetGraphicsRootDescriptorTable(2, meshData.material.textureIndex);
            srvManager_->SetGraphicsRootDescriptorTable(6, environmentSrvIndex);
        }
        else {
            // アニメーションあり - 頂点バッファ + スキニング用バッファ
            D3D12_VERTEX_BUFFER_VIEW vbvs[2] = {
                meshResource.vertexBufferView,
                skin_->GetSkinCluster().influenceBufferView };

            modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 2, vbvs);
            modelCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&meshResource.indexBufferView);
            srvManager_->SetGraphicsRootDescriptorTable(2, meshData.material.textureIndex);
            srvManager_->SetGraphicsRootDescriptorTable(7, skin_->GetSrvIndex());
        }
        srvManager_->SetGraphicsRootDescriptorTable(6, environmentSrvIndex);
        // --- 描画 ---
        modelCommon_->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(UINT(meshResource.indexCount), 1, 0, 0, 0);
    }
}

void Model::CreateVartexData() {
    meshResources_.resize(modelData.meshes.size());

    for (size_t i = 0; i < modelData.meshes.size(); ++i) {
        const auto& mesh = modelData.meshes[i];
        auto& meshResource = meshResources_[i];

        // 頂点リソースの作成
        meshResource.vertexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * mesh.vertices.size());
        meshResource.vertexBufferView.BufferLocation = meshResource.vertexResource->GetGPUVirtualAddress();
        meshResource.vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * mesh.vertices.size());
        meshResource.vertexBufferView.StrideInBytes = sizeof(VertexData);

        meshResource.vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&meshResource.vertexData));
        std::memcpy(meshResource.vertexData, mesh.vertices.data(), sizeof(VertexData) * mesh.vertices.size());
    }
}

void Model::CreateIndexResource() {
    for (size_t i = 0; i < modelData.meshes.size(); ++i) {
        const auto& mesh = modelData.meshes[i];
        auto& meshResource = meshResources_[i];

        // インデックスリソースの作成
        meshResource.indexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * mesh.indices.size());
        meshResource.indexBufferView.BufferLocation = meshResource.indexResource->GetGPUVirtualAddress();
        meshResource.indexBufferView.SizeInBytes = UINT(sizeof(uint32_t) * mesh.indices.size());
        meshResource.indexBufferView.Format = DXGI_FORMAT_R32_UINT;

        meshResource.indexResource->Map(0, nullptr, reinterpret_cast<void**>(&meshResource.indexData));
        std::memcpy(meshResource.indexData, mesh.indices.data(), sizeof(uint32_t) * mesh.indices.size());

        meshResource.indexCount = mesh.indices.size();
    }
}

std::map<std::string, MaterialData> Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
    std::map<std::string, MaterialData> materials;
    std::string line;
    std::string currentMaterialName;

    std::ifstream file(directoryPath + "/" + filename);
    assert(file.is_open());

    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;

        if (identifier == "newmtl") {
            s >> currentMaterialName;
            materials[currentMaterialName] = MaterialData();
        }
        else if (identifier == "map_Kd") {
            std::string textureFilename;
            s >> textureFilename;
            if (!currentMaterialName.empty()) {
                materials[currentMaterialName].textureFilePath = directoryPath + "/../images/" + textureFilename;
            }
        }
    }

    // テクスチャが設定されていないマテリアルにデフォルトテクスチャを割り当て
    for (auto& [name, material] : materials) {
        if (material.textureFilePath.empty()) {
            material.textureFilePath = "resources/images/white1x1.png";
        }
    }

    return materials;
}

ModelData Model::LoadModelFile(const std::string& directoryPath, const std::string& filename) {
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
    std::string filePath = directoryPath + filename;
    const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);

    if (!scene || !scene->HasMeshes()) {
        // テクスチャが設定されていない場合、"white1x1.png" を割り当てる
        MeshData defaultMesh;
        defaultMesh.material.textureFilePath = "resources/images/white1x1.png";
        modelData.meshes.push_back(defaultMesh);
        return modelData;
    }

    // モデルファイルの完全なパスからディレクトリ部分を取得
    std::string modelFileDir = directoryPath  + filename;
    size_t lastSlashPos = modelFileDir.find_last_of('/');
    if (lastSlashPos != std::string::npos) {
        modelFileDir = modelFileDir.substr(0, lastSlashPos + 1);
    }
    else {
        modelFileDir = directoryPath + "/";
    }

    // --- メッシュ処理 ---
    modelData.meshes.resize(scene->mNumMeshes);

    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        aiMesh* mesh = scene->mMeshes[meshIndex];
        MeshData& currentMesh = modelData.meshes[meshIndex];

        // 法線、Texcoordがない場合エラーを出力
        assert(mesh->HasNormals());
        assert(mesh->HasTextureCoords(0));

        currentMesh.vertices.resize(mesh->mNumVertices);
        for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
            aiVector3D& position = mesh->mVertices[vertexIndex];
            aiVector3D& normal = mesh->mNormals[vertexIndex];
            aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];
            currentMesh.vertices[vertexIndex].position = { -position.x, position.y, position.z, 1.0f };
            currentMesh.vertices[vertexIndex].normal = { -normal.x, normal.y, normal.z };
            currentMesh.vertices[vertexIndex].texcoord = { texcoord.x, texcoord.y };
        }

        for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
            aiFace& face = mesh->mFaces[faceIndex];
            assert(face.mNumIndices == 3);
            for (uint32_t element = 0; element < face.mNumIndices; ++element) {
                uint32_t vertexIndex = face.mIndices[element];
                currentMesh.indices.push_back(vertexIndex);
            }
        }

        // ボーン処理
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

            JointWeightData& jointWeightData = currentMesh.skinClusterData[jointName];
            jointWeightData.inverseBindPoseMatrix = Inverse(bindPoseMatrix);

            for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
                jointWeightData.vertexWeights.push_back({ bone->mWeights[weightIndex].mWeight,
                                                         bone->mWeights[weightIndex].mVertexId });
            }
        }

        // マテリアル処理 - メッシュごとに
        if (mesh->mMaterialIndex < scene->mNumMaterials) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
                aiString textureFilePath;
                material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);

                std::string texturePath = textureFilePath.C_Str();

                std::string imageDir = "resources/images/";

                // 元の相対パスをそのまま利用（../などは弾く）
                if (texturePath.find("..") == std::string::npos &&
                    texturePath.find(":") == std::string::npos)
                {
                    currentMesh.material.textureFilePath = imageDir + texturePath;
                }
                else {
                    // 絶対パスや異常なパスは弾く or fallback
                    std::string fileName = texturePath.substr(texturePath.find_last_of("/\\") + 1);
                    currentMesh.material.textureFilePath = imageDir + fileName;
                }
            }
        }

        // テクスチャが設定されていない場合のデフォルト
        if (currentMesh.material.textureFilePath.empty()) {
            currentMesh.material.textureFilePath = "resources/images/white1x1.png";
        }
    }

    // クリア
    jointNames.clear();

    modelData.rootNode = ReadNode(scene->mRootNode);
    return modelData;
}

Node Model::ReadNode(aiNode* node) {
    Node result;

    aiVector3D scale, translate;
    aiQuaternion rotate;
    node->mTransformation.Decompose(scale, rotate, translate);
    result.transform.scale = { scale.x, scale.y, scale.z };
    result.transform.rotate = { rotate.x, -rotate.y, -rotate.z, rotate.w };
    result.transform.translate = { -translate.x, translate.y, translate.z };

    result.localMatrix = MakeAffineMatrix(result.transform.scale, result.transform.rotate, result.transform.translate);

    result.name = node->mName.C_Str();
    result.children.resize(node->mNumChildren);
    for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
        result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
    }
    return result;
}