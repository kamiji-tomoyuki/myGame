#include "ParticleEmitter.h"
#include "line/DrawLine3D.h"

ParticleEmitter::ParticleEmitter() {}

void ParticleEmitter::Initialize(const std::string& name, const std::string& fileName)
{
	// --- 引数で受け取りメンバ変数に記録 ---
	name_ = name;

	transform_.Initialize();

	manager_ = std::make_unique<ParticleManager>();
	manager_->Initialize(SrvManager::GetInstance());
	manager_->CreateParticleGroup(name_, fileName);
	
	// --- 各ステータスにデフォルト値を設定 ---
	emitFrequency_ = 0.1f;
	velocityMin_ = { -1.0f, -1.0f, -1.0f };
	velocityMax_ = { 1.0f, 1.0f, 1.0f };
	lifeTimeMin_ = { 1.0f };
	lifeTimeMax_ = { 3.0f };
	isVisible = true;
	startAcce_ = { 1.0f,1.0f,1.0f };
	endAcce_ = { 1.0f,1.0f,1.0f };
	startScale_ = { 1.0f,1.0f,1.0f };
	endScale_ = { 1.0f,1.0f,1.0f };
	rotateVelocityMin = { -0.07f,-0.07f,-0.07f };
	rotateVelocityMax = { 0.07f,0.07f,0.07f };
	count_ = 3;
	alphaMin_ = 1.0f;
	alphaMax_ = 1.0f;
	AddItem();
	isBillBoard = false;
	isActive_ = true;
	isAcceMultiply = false;
	allScaleMin = { 1.0f,1.0f,1.0f };
	allScaleMax = { 1.0f,1.0f,1.0f };

	ApplyGlobalVariables();
}

void ParticleEmitter::Update(const ViewProjection& vp_) {
	SetValue();
	elapsedTime_ += deltaTime;

	while (elapsedTime_ >= emitFrequency_) {
		Emit();
		elapsedTime_ -= emitFrequency_;
	}

	manager_->Update(vp_);
	transform_.UpdateMatrix();
}

void ParticleEmitter::UpdateOnce(const ViewProjection& vp_)
{
	SetValue();
	if (!isActive_) {
		Emit();
		isActive_ = true;
	}
	manager_->Update(vp_);
	transform_.UpdateMatrix();
}

void ParticleEmitter::Draw()
{
	manager_->SetRandomRotate(isRandomRotate);
	manager_->SetAcceMultipy(isAcceMultiply);
	manager_->SetBillBorad(isBillBoard);
	manager_->SetRandomSize(isRandomScale);
	manager_->SetAllRandomSize(isAllRamdomScale);
	manager_->SetSinMove(isSinMove);
	manager_->Draw();
}

void ParticleEmitter::DrawEmitter()
{
	if (!isVisible) return;

	std::array<Vector3, 8> localVertices = {
		Vector3{-1.0f, -1.0f, -1.0f}, // 左下前
		Vector3{ 1.0f, -1.0f, -1.0f}, // 右下前
		Vector3{-1.0f,  1.0f, -1.0f}, // 左上前
		Vector3{ 1.0f,  1.0f, -1.0f}, // 右上前
		Vector3{-1.0f, -1.0f,  1.0f}, // 左下奥
		Vector3{ 1.0f, -1.0f,  1.0f}, // 右下奥
		Vector3{-1.0f,  1.0f,  1.0f}, // 左上奥
		Vector3{ 1.0f,  1.0f,  1.0f}  // 右上奥
	};

	std::array<Vector3, 8> worldVertices;

	Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale_, transform_.rotation_, transform_.translation_);

	for (size_t i = 0; i < localVertices.size(); i++) {
		worldVertices[i] = Transformation(localVertices[i], worldMatrix);
	}

	constexpr std::array<std::pair<int, int>, 12> edges = {
		std::make_pair(0, 1), std::make_pair(1, 3), std::make_pair(3, 2), std::make_pair(2, 0), // 前面
		std::make_pair(4, 5), std::make_pair(5, 7), std::make_pair(7, 6), std::make_pair(6, 4), // 背面
		std::make_pair(0, 4), std::make_pair(1, 5), std::make_pair(2, 6), std::make_pair(3, 7)  // 側面
	};

	for (const auto& edge : edges) {
		DrawLine3D::GetInstance()->SetPoints(worldVertices[edge.first], worldVertices[edge.second]);
	}
}


void ParticleEmitter::Emit() {
		manager_->Emit(
		name_,
		transform_.translation_,
		count_,
		transform_.scale_,
		velocityMin_,     
		velocityMax_,     
		lifeTimeMin_,     
		lifeTimeMax_,     
		startScale_,
		endScale_,
		startAcce_,
		endAcce_,
		startRote_,
		endRote_,
		isRandomColor,
		alphaMin_,
		alphaMax_,
		rotateVelocityMin,
		rotateVelocityMax,
		allScaleMax,
		allScaleMin,
		scaleMin,
		scaleMax,
		transform_.rotation_
	);
}

void ParticleEmitter::ApplyGlobalVariables()
{
	emitFrequency_ = globalVariables->GetFloatValue(groupName, "emitFrequency");
	count_ = globalVariables->GetIntValue(groupName, "count");
	transform_.translation_ = globalVariables->GetVector3Value(groupName, "Emit translation");
	transform_.scale_ = globalVariables->GetVector3Value(groupName, "Emit scale");
	transform_.rotation_ = globalVariables->GetVector3Value(groupName, "Emit rotation");
	startScale_ = globalVariables->GetVector3Value(groupName, "Particle StartScale");
	endScale_ = globalVariables->GetVector3Value(groupName, "Particle EndScale");
	startRote_ = globalVariables->GetVector3Value(groupName, "Particle StartRote");
	endRote_ = globalVariables->GetVector3Value(groupName, "Particle EndRote");
	startAcce_ = globalVariables->GetVector3Value(groupName, "Particle StartAcce");
	endAcce_ = globalVariables->GetVector3Value(groupName, "Particle EndAcce");
	velocityMin_ = globalVariables->GetVector3Value(groupName, "minVelocity");
	velocityMax_ = globalVariables->GetVector3Value(groupName, "maxVelocity");
	lifeTimeMax_ = globalVariables->GetFloatValue(groupName, "lifeTimeMax");
	lifeTimeMin_ = globalVariables->GetFloatValue(groupName, "lifeTimeMin");
	isVisible = globalVariables->GetBoolValue(groupName, "isVisible");
	isBillBoard = globalVariables->GetBoolValue(groupName, "isBillBoard");
	isRandomColor = globalVariables->GetBoolValue(groupName, "isRamdomColor");
	alphaMin_ = globalVariables->GetFloatValue(groupName, "alphaMin");
	alphaMax_ = globalVariables->GetFloatValue(groupName, "alphaMax");
	isRandomRotate = globalVariables->GetBoolValue(groupName, "isRandomRotate");
	isAcceMultiply = globalVariables->GetBoolValue(groupName, "isAcceMultiply");
	rotateVelocityMin = globalVariables->GetVector3Value(groupName, "RotationVelo Min");
	rotateVelocityMax = globalVariables->GetVector3Value(groupName, "RotationVelo Max");
	allScaleMax = globalVariables->GetVector3Value(groupName, "AllScale Max");
	allScaleMin = globalVariables->GetVector3Value(groupName, "AllScale Min");
	scaleMin = globalVariables->GetFloatValue(groupName, "Scale Min");
	scaleMax = globalVariables->GetFloatValue(groupName, "Scale Max");
	isRandomScale = globalVariables->GetBoolValue(groupName, "isRandomScale");
	isAllRamdomScale = globalVariables->GetBoolValue(groupName, "isAllRamdomScale");
	isSinMove = globalVariables->GetBoolValue(groupName, "isSinMove");
}

void ParticleEmitter::SetValue()
{
	globalVariables->SetValue(groupName, "emitFrequency", emitFrequency_);
	globalVariables->SetValue(groupName, "count", count_);
	globalVariables->SetValue(groupName, "Emit translation", transform_.translation_);
	globalVariables->SetValue(groupName, "Emit scale", transform_.scale_);
	globalVariables->SetValue(groupName, "Emit rotation", transform_.rotation_);
	globalVariables->SetValue(groupName, "Particle StartScale", startScale_);
	globalVariables->SetValue(groupName, "Particle StartRote", startRote_);
	globalVariables->SetValue(groupName, "Particle EndRote", endRote_);
	globalVariables->SetValue(groupName, "Particle EndScale", endScale_);
	globalVariables->SetValue(groupName, "Particle StartAcce", startAcce_);
	globalVariables->SetValue(groupName, "Particle EndAcce", endAcce_);
	globalVariables->SetValue(groupName, "minVelocity", velocityMin_);
	globalVariables->SetValue(groupName, "maxVelocity", velocityMax_);
	globalVariables->SetValue(groupName, "lifeTimeMax", lifeTimeMax_);
	globalVariables->SetValue(groupName, "lifeTimeMin", lifeTimeMin_);
	globalVariables->SetValue(groupName, "isVisible", isVisible);
	globalVariables->SetValue(groupName, "isBillBoard", isBillBoard);
	globalVariables->SetValue(groupName, "isRamdomColor", isRandomColor);
	globalVariables->SetValue(groupName, "alphaMin", alphaMin_);
	globalVariables->SetValue(groupName, "alphaMax", alphaMax_);
	globalVariables->SetValue(groupName, "isRandomRotate", isRandomRotate);
	globalVariables->SetValue(groupName, "RotationVelo Min", rotateVelocityMin);
	globalVariables->SetValue(groupName, "isAcceMultiply", isAcceMultiply);
	globalVariables->SetValue(groupName, "RotationVelo Max", rotateVelocityMax);
	globalVariables->SetValue(groupName, "AllScale Max", allScaleMax);
	globalVariables->SetValue(groupName, "AllScale Min", allScaleMin);
	globalVariables->SetValue(groupName, "Scale Min", scaleMin);
	globalVariables->SetValue(groupName, "Scale Max", scaleMax);
	globalVariables->SetValue(groupName, "isRandomScale", isRandomScale);
	globalVariables->SetValue(groupName, "isAllRamdomScale", isAllRamdomScale);
	globalVariables->SetValue(groupName, "isSinMove", isSinMove);
}

void ParticleEmitter::AddItem()
{
	groupName = name_.c_str();
	globalVariables = GlobalVariables::GetInstance();
	globalVariables->CreateGroup(groupName);
	globalVariables->AddItem(groupName, "emitFrequency", emitFrequency_);
	globalVariables->AddItem(groupName, "count", count_);
	globalVariables->AddItem(groupName, "Emit translation", transform_.translation_);
	globalVariables->AddItem(groupName, "Emit scale", transform_.scale_);
	globalVariables->AddItem(groupName, "Emit rotation", transform_.rotation_);
	globalVariables->AddItem(groupName, "Particle StartScale", startScale_);
	globalVariables->AddItem(groupName, "Particle EndScale", endScale_);
	globalVariables->AddItem(groupName, "Particle StartRote", startRote_);
	globalVariables->AddItem(groupName, "Particle EndRote", endRote_);
	globalVariables->AddItem(groupName, "Particle StartAcce", startAcce_);
	globalVariables->AddItem(groupName, "Particle EndAcce", endAcce_);
	globalVariables->AddItem(groupName, "minVelocity", velocityMin_);
	globalVariables->AddItem(groupName, "maxVelocity", velocityMax_);
	globalVariables->AddItem(groupName, "lifeTimeMax", lifeTimeMax_);
	globalVariables->AddItem(groupName, "lifeTimeMin", lifeTimeMin_);
	globalVariables->AddItem(groupName, "isRamdomColor", isRandomColor);
	globalVariables->AddItem(groupName, "alphaMin", alphaMin_);
	globalVariables->AddItem(groupName, "alphaMax", alphaMax_);
	globalVariables->AddItem(groupName, "AllScale Max", allScaleMax);
	globalVariables->AddItem(groupName, "AllScale Min", allScaleMin);
	globalVariables->AddItem(groupName, "Scale Min", scaleMin);
	globalVariables->AddItem(groupName, "Scale Max", scaleMax);
	globalVariables->AddItem(groupName, "isVisible", isVisible);
	globalVariables->AddItem(groupName, "isBillBoard", isBillBoard);
	globalVariables->AddItem(groupName, "isRandomRotate", isRandomRotate);
	globalVariables->AddItem(groupName, "isAcceMultiply", isAcceMultiply);
	globalVariables->AddItem(groupName, "RotationVelo Min", rotateVelocityMin);
	globalVariables->AddItem(groupName, "RotationVelo Max", rotateVelocityMax);
	globalVariables->AddItem(groupName, "isRandomScale", isRandomScale);
	globalVariables->AddItem(groupName, "isAllRamdomScale", isAllRamdomScale);
	globalVariables->AddItem(groupName, "isSinMove", isSinMove);
}

void ParticleEmitter::imgui() {
#ifdef _DEBUG
	ImGui::Begin(name_.c_str());

	if (ImGui::CollapsingHeader("エミッターデータ")) {
		ImGui::Text("Transformデータ:");
		ImGui::Separator();
		ImGui::Columns(2, "TransformColumns", false);
		ImGui::Text("位置"); ImGui::NextColumn();
		ImGui::DragFloat3("##位置", &transform_.translation_.x, 0.1f);
		ImGui::NextColumn();
		ImGui::Text("回転"); ImGui::NextColumn();
		float rotationDegrees[3] = {
		radiansToDegrees(transform_.rotation_.x),
		radiansToDegrees(transform_.rotation_.y),
		radiansToDegrees(transform_.rotation_.z)
		};

		if (ImGui::DragFloat3("##回転 (度)", rotationDegrees, 0.1f, -360.0f, 360.0f)) {
			transform_.rotation_.x = degreesToRadians(rotationDegrees[0]);
			transform_.rotation_.y = degreesToRadians(rotationDegrees[1]);
			transform_.rotation_.z = degreesToRadians(rotationDegrees[2]);
		}

		ImGui::NextColumn();
		ImGui::Text("大きさ"); ImGui::NextColumn();
		ImGui::DragFloat3("##大きさ", &transform_.scale_.x, 0.1f, 0.0f);
		ImGui::Columns(1);
		ImGui::Separator();

		ImGui::Checkbox("表示", &isVisible);
	}

	if (ImGui::CollapsingHeader("パーティクルデータ")) {
		if (ImGui::TreeNode("寿命")) {
			ImGui::Text("寿命設定:");
			ImGui::Separator();
			ImGui::DragFloat("最大値", &lifeTimeMax_, 0.1f, 0.0f);
			ImGui::DragFloat("最小値", &lifeTimeMin_, 0.1f, 0.0f);
			lifeTimeMin_ = std::clamp(lifeTimeMin_, 0.0f, lifeTimeMax_);
			lifeTimeMax_ = std::clamp(lifeTimeMax_, lifeTimeMin_, 10.0f);
			ImGui::TreePop();
		}

		ImGui::Separator();

		if (ImGui::TreeNode("速度、加速度")) {
			ImGui::Text("速度:");
			ImGui::DragFloat3("最大値", &velocityMax_.x, 0.1f);
			ImGui::DragFloat3("最小値", &velocityMin_.x, 0.1f);
			velocityMin_.x = std::clamp(velocityMin_.x, -FLT_MAX, velocityMax_.x);
			velocityMax_.x = std::clamp(velocityMax_.x, velocityMin_.x, FLT_MAX);
			velocityMin_.y = std::clamp(velocityMin_.y, -FLT_MAX, velocityMax_.y);
			velocityMax_.y = std::clamp(velocityMax_.y, velocityMin_.y, FLT_MAX);
			velocityMin_.z = std::clamp(velocityMin_.z, -FLT_MAX, velocityMax_.z);
			velocityMax_.z = std::clamp(velocityMax_.z, velocityMin_.z, FLT_MAX);
			ImGui::Text("加速度:");
			ImGui::DragFloat3("最初", &startAcce_.x, 0.001f);
			ImGui::DragFloat3("最後", &endAcce_.x, 0.001f);
			ImGui::Checkbox("乗算", &isAcceMultiply);
			ImGui::TreePop();
		}

		ImGui::Separator();

		if (ImGui::TreeNode("大きさ")) {
			ImGui::Text("大きさ:");
			if (isAllRamdomScale) {
				ImGui::DragFloat3("最大値", &allScaleMax.x, 0.1f, 0.0f);
				ImGui::DragFloat3("最小値", &allScaleMin.x, 0.1f, 0.0f);
				allScaleMin.x = std::clamp(allScaleMin.x, -FLT_MAX, allScaleMax.x);
				allScaleMax.x = std::clamp(allScaleMax.x, allScaleMin.x, FLT_MAX);
				allScaleMin.y = std::clamp(allScaleMin.y, -FLT_MAX, allScaleMax.y);
				allScaleMax.y = std::clamp(allScaleMax.y, allScaleMin.y, FLT_MAX);
				allScaleMin.z = std::clamp(allScaleMin.z, -FLT_MAX, allScaleMax.z);
				allScaleMax.z = std::clamp(allScaleMax.z, allScaleMin.z, FLT_MAX);
			}
			else if (isRandomScale) {
				ImGui::DragFloat("最大値", &scaleMax, 0.1f, 0.0f);
				ImGui::DragFloat("最小値", &scaleMin, 0.1f, 0.0f);
				scaleMax = std::clamp(scaleMax, scaleMin, FLT_MAX);
				scaleMin = std::clamp(scaleMin, 0.0f, scaleMax);
			}
			else if (isSinMove) {
				ImGui::DragFloat3("最初", &startScale_.x, 0.1f, 0.0f);
			}
			else {
				ImGui::DragFloat3("最初", &startScale_.x, 0.1f, 0.0f);
			}
			if (!isSinMove) {
				ImGui::DragFloat3("最後", &endScale_.x, 0.1f);
			}
			ImGui::Checkbox("均等にランダムな大きさ", &isRandomScale);
			ImGui::Checkbox("ばらばらにランダムな大きさ", &isAllRamdomScale);
			ImGui::Checkbox("sin波の動き", &isSinMove);
			ImGui::TreePop();
		}

		ImGui::Separator();

		if (ImGui::TreeNode("回転")) {
			if (!isRandomRotate) {
				float startRotationDegrees[3] = {
				  radiansToDegrees(startRote_.x),
				  radiansToDegrees(startRote_.y),
				  radiansToDegrees(startRote_.z)
				};
				float endRotationDegrees[3] = {
				  radiansToDegrees(endRote_.x),
				  radiansToDegrees(endRote_.y),
				  radiansToDegrees(endRote_.z)
				};
				if (ImGui::DragFloat3("最初", startRotationDegrees, 0.1f)) {
					startRote_.x = degreesToRadians(startRotationDegrees[0]);
					startRote_.y = degreesToRadians(startRotationDegrees[1]);
					startRote_.z = degreesToRadians(startRotationDegrees[2]);
				}
				if (ImGui::DragFloat3("最後", endRotationDegrees, 0.1f)) {
					endRote_.x = degreesToRadians(endRotationDegrees[0]);
					endRote_.y = degreesToRadians(endRotationDegrees[1]);
					endRote_.z = degreesToRadians(endRotationDegrees[2]);
				}
			}
			if (isRandomRotate) {
				ImGui::DragFloat3("最大値", &rotateVelocityMax.x, 0.01f);
				ImGui::DragFloat3("最小値", &rotateVelocityMin.x, 0.01f);
				rotateVelocityMin.x = std::clamp(rotateVelocityMin.x, -FLT_MAX, rotateVelocityMax.x);
				rotateVelocityMax.x = std::clamp(rotateVelocityMax.x, rotateVelocityMin.x, FLT_MAX);
				rotateVelocityMin.y = std::clamp(rotateVelocityMin.y, -FLT_MAX, rotateVelocityMax.y);
				rotateVelocityMax.y = std::clamp(rotateVelocityMax.y, rotateVelocityMin.y, FLT_MAX);
				rotateVelocityMin.z = std::clamp(rotateVelocityMin.z, -FLT_MAX, rotateVelocityMax.z);
				rotateVelocityMax.z = std::clamp(rotateVelocityMax.z, rotateVelocityMin.z, FLT_MAX);
			}
			ImGui::Checkbox("ランダムな回転", &isRandomRotate);
			ImGui::TreePop();
		}

		ImGui::Separator();

		if (ImGui::TreeNode("透明度")) {
			ImGui::Text("透明度の設定:");
			ImGui::DragFloat("最大値", &alphaMax_, 0.1f, 0.0f, 1.0f);
			ImGui::DragFloat("最小値", &alphaMin_, 0.1f, 0.0f, 1.0f);
			alphaMin_ = std::clamp(alphaMin_, 0.0f, alphaMax_);
			alphaMax_ = std::clamp(alphaMax_, alphaMin_, 1.0f);
			ImGui::TreePop();
		}
	}

	if (ImGui::CollapsingHeader("パーティクルの数、間隔")) {
		ImGui::DragFloat("間隔", &emitFrequency_, 0.1f, 0.1f, 100.0f);
		ImGui::InputInt("数", &count_, 1, 100);
		count_ = std::clamp(count_, 0, 10000);
	}

	if (ImGui::CollapsingHeader("各状態の設定")) {
		ImGui::Checkbox("ビルボード", &isBillBoard);
		ImGui::Checkbox("ランダムカラー", &isRandomColor);
	}

	ImGui::End();

#endif
}
