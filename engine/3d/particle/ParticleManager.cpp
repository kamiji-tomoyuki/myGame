#include "ParticleManager.h"
#include "TextureManager.h"
#include "fstream"

std::unordered_map<std::string, ParticleManager::ModelData> ParticleManager::modelCache;

void ParticleManager::Initialize(SrvManager* srvManager)
{
	particleCommon = ParticleCommon::GetInstance();
	srvManager_ = srvManager;
	randomEngine.seed(seedGenerator());
}

void ParticleManager::Update(const ViewProjection& viewProjection)
{
	// --- 各行列の初期化・計算 ---
	Matrix4x4 viewProjectionMatrix = viewProjection.matView_ * viewProjection.matProjection_;

	Matrix4x4 billboardMatrix = viewProjection.matView_;
	billboardMatrix.m[3][0] = 0.0f;
	billboardMatrix.m[3][1] = 0.0f;
	billboardMatrix.m[3][2] = 0.0f;
	billboardMatrix.m[3][3] = 1.0f;

	billboardMatrix = Inverse(billboardMatrix);

	for (auto& [groupName, particleGroup] : particleGroups) {

		uint32_t numInstance = 0;

		// --- パーティクルの更新 ---
		for (auto particleIterator = particleGroup.particles.begin();
			particleIterator != particleGroup.particles.end();) {

			if ((*particleIterator).lifeTime <= (*particleIterator).currentTime) {
				particleIterator = particleGroup.particles.erase(particleIterator);
				continue;
			}
			// パーティクルの生存時間 t の計算
			float t = (*particleIterator).currentTime / (*particleIterator).lifeTime;
			t = std::clamp(t, 0.0f, 1.0f);

			// --- 拡縮処理 ---
			if (isSinMove_) {
				// Sin波の周波数制御 (速度調整)
				float waveScale = 0.5f * (sin(t * DirectX::XM_PI * 18.0f) + 1.0f);  // 0 ~ 1

				// 最大スケールが寿命に応じて縮小し、最終的に0になる
				float maxScale = (1.0f - t); 

				// Sin波スケールと最大スケールの積を適用
				(*particleIterator).transform.scale_ =
					(*particleIterator).startScale * waveScale * maxScale;

			} else {
				// 通常の線形補間
				(*particleIterator).transform.scale_ =
					(1.0f - t) * (*particleIterator).startScale + t * (*particleIterator).endScale;
				
				// アルファ値の計算
				(*particleIterator).color.w = (*particleIterator).initialAlpha - ((*particleIterator).currentTime / (*particleIterator).lifeTime);
			}

			(*particleIterator).Acce = (1.0f - t) * (*particleIterator).startAcce + t * (*particleIterator).endAcce;

			if (isRandomRotate_) {
				(*particleIterator).transform.rotation_ += (*particleIterator).rotateVelocity;
			} else {
				(*particleIterator).transform.rotation_ = (1.0f - t) * (*particleIterator).startRote + t * (*particleIterator).endRote;
			}

			if (isAcceMultipy_) {
				(*particleIterator).velocity *= (*particleIterator).Acce;
			} else {
				(*particleIterator).velocity += (*particleIterator).Acce;
			}

			// パーティクルの移動
			(*particleIterator).transform.translation_ +=
				(*particleIterator).velocity * kDeltaTime;
			(*particleIterator).currentTime += kDeltaTime;

			// ワールド行列の計算
			Matrix4x4 worldMatrix{};

			if (isBillboard) {
				// ビルボード
				worldMatrix = MakeScaleMatrix((*particleIterator).transform.scale_) * billboardMatrix *
					MakeTranslateMatrix((*particleIterator).transform.translation_);
			} else {
				// 通常
				worldMatrix = MakeAffineMatrix((*particleIterator).transform.scale_,
					(*particleIterator).transform.rotation_,
					(*particleIterator).transform.translation_);
			}

			// ワールド・ビュー・プロジェクション行列計算
			Matrix4x4 worldViewProjectionMatrix = worldMatrix * viewProjectionMatrix;

			// インスタンスデータ設定
			if (numInstance < kNumMaxInstance) {
				particleGroup.instancingData[numInstance].WVP = worldViewProjectionMatrix;
				particleGroup.instancingData[numInstance].World = worldMatrix;
				particleGroup.instancingData[numInstance].color = (*particleIterator).color;
				particleGroup.instancingData[numInstance].color.w = (*particleIterator).color.w;
				++numInstance;
			}

			++particleIterator;
		}

		// インスタンス数更新
		particleGroup.instanceCount = numInstance;
	}
}

void ParticleManager::Draw()
{
	particleCommon->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

	for (auto& [groupName, particleGroup] : particleGroups) {
		if (particleGroup.instanceCount > 0) {
			particleCommon->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

			srvManager_->SetGraphicsRootDescriptorTable(1, particleGroup.instancingSRVIndex);
			srvManager_->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetTextureIndexByFilePath(particleGroup.material.textureFilePath));

			particleCommon->GetDxCommon()->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), particleGroup.instanceCount, 0, 0);
		}
	}
}

void ParticleManager::CreateParticleGroup(const std::string name, const std::string& filename)
{
	// --- パーティクルグループ生成 ---
	if (particleGroups.contains(name)) {
		return;
	}

	particleGroups[name] = ParticleGroup();
	ParticleGroup& particleGroup = particleGroups[name];
	CreateVartexData(filename);

	particleGroup.material.textureFilePath = modelData.material.textureFilePath;
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	particleGroup.instancingResource = particleCommon->GetDxCommon()->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance);

	particleGroup.instancingSRVIndex = srvManager_->Allocate() + 1;
	particleGroup.instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&particleGroup.instancingData));

	srvManager_->CreateSRVforStructuredBuffer(particleGroup.instancingSRVIndex, particleGroup.instancingResource.Get(), kNumMaxInstance, sizeof(ParticleForGPU));

	CreateMaterial();
	particleGroup.instanceCount = 0;
}

void ParticleManager::CreateVartexData(const std::string& filename)
{
	modelData = LoadObjFile("resources/models/", filename);

	// --- 頂点リソース生成 ---
	vertexResource = particleCommon->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
	// --- 頂点バッファビュー生成 ---
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	// --- 書き込み ---
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
}

ParticleManager::Particle ParticleManager::MakeNewParticle(
	std::mt19937& randomEngine,
	const Vector3& translate,
	const Vector3& rotation,
	const Vector3& scale,
	const Vector3& velocityMin, const Vector3& velocityMax,
	float lifeTimeMin, float lifeTimeMax,
	const Vector3& particleStartScale, const Vector3& particleEndScale,
	const Vector3& startAcce, const Vector3& endAcce,
	const Vector3& startRote, const Vector3& endRote,
	bool isRamdomColor, float alphaMin, float alphaMax,
	const Vector3& rotateVelocityMin, const Vector3& rotateVelocityMax,
	const Vector3& allScaleMax, const Vector3& allScaleMin,
	const float& scaleMin, const float& scaleMax
)
{
	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
	std::uniform_real_distribution<float> distVelocityX(velocityMin.x, velocityMax.x);
	std::uniform_real_distribution<float> distVelocityY(velocityMin.y, velocityMax.y);
	std::uniform_real_distribution<float> distVelocityZ(velocityMin.z, velocityMax.z);
	std::uniform_real_distribution<float> distLifeTime(lifeTimeMin, lifeTimeMax);
	std::uniform_real_distribution<float> distAlpha(alphaMin, alphaMax);

	Particle particle;

	// スケールを考慮して位置を生成
	Vector3 randomTranslate = {
		distribution(randomEngine) * scale.x,
		distribution(randomEngine) * scale.y,
		distribution(randomEngine) * scale.z
	};

	// 回転行列を適用しランダムに回転
	Matrix4x4 rotationMatrix = MakeRotateXYZMatrix(rotation);

	// ランダム位置を回転行列で変換
	Vector3 rotatedPosition = {
		randomTranslate.x * rotationMatrix.m[0][0] + randomTranslate.y * rotationMatrix.m[1][0] + randomTranslate.z * rotationMatrix.m[2][0],
		randomTranslate.x * rotationMatrix.m[0][1] + randomTranslate.y * rotationMatrix.m[1][1] + randomTranslate.z * rotationMatrix.m[2][1],
		randomTranslate.x * rotationMatrix.m[0][2] + randomTranslate.y * rotationMatrix.m[1][2] + randomTranslate.z * rotationMatrix.m[2][2]
	};

	// 回転されたランダムな位置をトランスレーションに加算
	particle.transform.translation_ = translate + rotatedPosition;

	if (isRandomAllSize_) {
		std::uniform_real_distribution<float> distScaleX(allScaleMin.x, allScaleMax.x);
		std::uniform_real_distribution<float> distScaleY(allScaleMin.y, allScaleMax.y);
		std::uniform_real_distribution<float> distScaleZ(allScaleMin.z, allScaleMax.z);

		particle.startScale = { distScaleX(randomEngine),distScaleY(randomEngine),distScaleZ(randomEngine) };
	} else if (isRandomSize_) {
		std::uniform_real_distribution<float> distScale(scaleMin, scaleMax);
		particle.startScale.x = distScale(randomEngine);
		particle.startScale.y = particle.startScale.x;
		particle.startScale.z = particle.startScale.x;

	} else {
		particle.startScale = particleStartScale;
	}

	particle.endScale = particleEndScale;

	particle.startAcce = startAcce;
	particle.endAcce = endAcce;

	// パーティクルの速度をランダムに設定
	Vector3 randomVelocity = {
		distVelocityX(randomEngine),
		distVelocityY(randomEngine),
		distVelocityZ(randomEngine)
	};

	// エミッターの回転を速度ベクトルに適用
	particle.velocity = {
		randomVelocity.x * rotationMatrix.m[0][0] + randomVelocity.y * rotationMatrix.m[1][0] + randomVelocity.z * rotationMatrix.m[2][0],
		randomVelocity.x * rotationMatrix.m[0][1] + randomVelocity.y * rotationMatrix.m[1][1] + randomVelocity.z * rotationMatrix.m[2][1],
		randomVelocity.x * rotationMatrix.m[0][2] + randomVelocity.y * rotationMatrix.m[1][2] + randomVelocity.z * rotationMatrix.m[2][2]
	};


	if (isRandomRotate_) {
		// 回転速度をランダムに設定
		std::uniform_real_distribution<float> distRotateXVelocity(rotateVelocityMin.x, rotateVelocityMax.x);
		std::uniform_real_distribution<float> distRotateYVelocity(rotateVelocityMin.y, rotateVelocityMax.y);
		std::uniform_real_distribution<float> distRotateZVelocity(rotateVelocityMin.z, rotateVelocityMax.z);
		std::uniform_real_distribution<float> distRotateX(0.0f, 2.0f);
		std::uniform_real_distribution<float> distRotateY(0.0f, 2.0f);
		std::uniform_real_distribution<float> distRotateZ(0.0f, 2.0f);
		particle.rotateVelocity.x = distRotateXVelocity(randomEngine);
		particle.rotateVelocity.y = distRotateYVelocity(randomEngine);
		particle.rotateVelocity.z = distRotateZVelocity(randomEngine);
		particle.transform.rotation_.x = distRotateX(randomEngine);
		particle.transform.rotation_.y = distRotateY(randomEngine);
		particle.transform.rotation_.z = distRotateZ(randomEngine);
	} else {
		particle.startRote = startRote;
		particle.endRote = endRote;
	}

	if (isRamdomColor) {
		std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
		particle.color = { distColor(randomEngine), distColor(randomEngine), distColor(randomEngine), distAlpha(randomEngine) };
	} else {
		particle.color = { 1.0f,1.0f,1.0f, distAlpha(randomEngine) };
	}

	particle.initialAlpha = distAlpha(randomEngine);
	particle.lifeTime = distLifeTime(randomEngine);
	particle.currentTime = 0.0f;

	return particle;
}

ParticleManager::MaterialData ParticleManager::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename)
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
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}

	if (materialData.textureFilePath.empty()) {
		materialData.textureFilePath = directoryPath + "/../images/white1x1.png";
	}

	return materialData;
}


ParticleManager::ModelData ParticleManager::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	// --- .obj読み込み ---
	std::string fullPath = directoryPath + filename;

	auto it = modelCache.find(fullPath);
	if (it != modelCache.end()) {
		return it->second;
	}

	ModelData modelData;
	std::vector<Vector4> positions;
	std::vector<Vector2> texcoords;
	std::string line;

	std::string folderPath;
	size_t lastSlashPos = filename.find_last_of("/\\");
	if (lastSlashPos != std::string::npos) {
		folderPath = filename.substr(0, lastSlashPos);
	}

	std::ifstream file(fullPath);
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.x *= -1.0f;
			position.w = 1.0f;
			positions.push_back(position);
		}
		else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);
		}
		else if (identifier == "f") {
			VertexData triangle[3];
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/');
					elementIndices[element] = std::stoi(index);
				}
				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				VertexData vertex = { position, texcoord };
				modelData.vertices.push_back(vertex);
				triangle[faceVertex] = { position, texcoord };
			}
		}
		else if (identifier == "mtllib") {
			std::string materialFilename;
			s >> materialFilename;
			if (!folderPath.empty()) {
				modelData.material = LoadMaterialTemplateFile(directoryPath + folderPath, materialFilename);
			}
			else {
				modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
			}
		}
	}

	modelCache[fullPath] = modelData;

	return modelData;
}


void ParticleManager::CreateMaterial()
{
	// --- マテリアルリソース生成 ---
	materialResource = particleCommon->GetDxCommon()->CreateBufferResource(sizeof(Material));
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->uvTransform = MakeIdentity4x4();
}

std::list<ParticleManager::Particle> ParticleManager::Emit(
	const std::string name,
	const Vector3& position,
	uint32_t count,
	const Vector3& scale,
	const Vector3& velocityMin, const Vector3& velocityMax,
	float lifeTimeMin, float lifeTimeMax,
	const Vector3& particleStartScale, const Vector3& particleEndScale,
	const Vector3& startAcce, const Vector3& endAcce,
	const Vector3& startRote, const Vector3& endRote,
	bool isRandomColor, float alphaMin, float alphaMax,
	const Vector3& rotateVelocityMin, const Vector3& rotateVelocityMax,
	const Vector3& allScaleMax, const Vector3& allScaleMin,
	const float& scaleMin, const float& scaleMax, const Vector3& rotation)
{
	assert(particleGroups.find(name) != particleGroups.end() && "Error: パーティクルグループが存在しません。");

	ParticleGroup& particleGroup = particleGroups[name];

	std::list<Particle> newParticles;
	for (uint32_t nowCount = 0; nowCount < count; ++nowCount) {
		Particle particle = MakeNewParticle(
			randomEngine,
			position,
			rotation,
			scale,
			velocityMin,
			velocityMax,
			lifeTimeMin,
			lifeTimeMax,
			particleStartScale,
			particleEndScale,
			startAcce,
			endAcce,
			startRote,
			endRote,
			isRandomColor,
			alphaMin,
			alphaMax,
			rotateVelocityMin,
			rotateVelocityMax,
			allScaleMax, allScaleMin,
			scaleMin, scaleMax
		);
		newParticles.push_back(particle);
	}

	particleGroup.particles.splice(particleGroup.particles.end(), newParticles);

	return newParticles;
}
