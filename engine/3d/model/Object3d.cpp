#include "Object3d.h"
#include "Object3dCommon.h"
#include "ModelManager.h"
#include "AnimationManager.h"
#include "cassert"

#include <line/DrawLine3D.h>

#include "myMath.h"

void Object3d::Initialize(const std::string& filePath)
{
	this->obj3dCommon = Object3dCommon::GetInstance();
	lightGroup = LightGroup::GetInstance();

	CreateTransformationMatrix();
	CreateMaterial();

	ModelManager::GetInstance()->LoadModel(filePath);
	model = ModelManager::GetInstance()->FindModel(filePath);

	modelAnimation_ = std::make_unique<ModelAnimation>();
	modelAnimation_->SetModelData(model->GetModelData());
	modelAnimation_->Initialize("resources/models/", filePath);

	model->SetAnimator(modelAnimation_->GetAnimator());
	model->SetBone(modelAnimation_->GetBone());
	model->SetSkin(modelAnimation_->GetSkin());
}

void Object3d::Update(const WorldTransform& worldTransform, const ViewProjection& viewProjection)
{
	if (lightGroup) {
		lightGroup->Update(viewProjection);
	}
	Matrix4x4 worldMatrix = MakeAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);

	if (worldTransform.parent_) {
		worldMatrix *= worldTransform.parent_->matWorld_;
	}
	Matrix4x4 worldViewProjectionMatrix;
	const Matrix4x4& viewProjectionMatrix = viewProjection.matView_ * viewProjection.matProjection_;
	worldViewProjectionMatrix = worldMatrix * viewProjectionMatrix;

	Matrix4x4 worldInverseMatrix = Inverse(worldMatrix);
	transformationMatrixData->WVP = worldViewProjectionMatrix;
	transformationMatrixData->World = worldTransform.matWorld_;
	transformationMatrixData->WorldInverseTranspose = Transpose(worldInverseMatrix);
}

void Object3d::AnimationUpdate(bool roop)
{
	if (modelAnimation_) {
		modelAnimation_->Update(roop);
	}
}

void Object3d::SetAnimation(const std::string& fileName)
{
	modelAnimation_ = std::make_unique<ModelAnimation>();
	modelAnimation_->SetModelData(model->GetModelData());
	modelAnimation_->Initialize("resources/models/", fileName);
	modelAnimation_->GetAnimator()->SetAnimationTime(0.0f);
	model->SetAnimator(modelAnimation_->GetAnimator());
	model->SetBone(modelAnimation_->GetBone());
	model->SetSkin(modelAnimation_->GetSkin());
}

void Object3d::Draw(const WorldTransform& worldTransform, const ViewProjection& viewProjection, ObjColor* color, bool Lighting)
{
	if (color) {
		materialData->color = color->GetColor();
	}
	materialData->enableLighting = Lighting;
	Update(worldTransform, viewProjection);

	obj3dCommon->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	obj3dCommon->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());
	
	if (materialData->enableLighting != 0 && lightGroup) {
		lightGroup->Draw();
	}
	
	if (model) {
		model->Draw();
	}
}

void Object3d::DrawSkeleton(const WorldTransform& worldTransform, const ViewProjection& viewProjection)
{
	Update(worldTransform, viewProjection);
	
	const Skeleton& skeleton = modelAnimation_->GetSkeletonData();

	for (const auto& joint : skeleton.joints) {
		// 親がいない場合スキップ
		if (!joint.parent.has_value()) {
			continue;
		}

		const auto& parentJoint = skeleton.joints[*joint.parent];

		Vector3 parentPosition = ExtractTranslation(parentJoint.skeletonSpaceMatrix);
		Vector3 childPosition = ExtractTranslation(joint.skeletonSpaceMatrix);

		Vector4 lineColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		DrawLine3D::GetInstance()->SetPoints(parentPosition, childPosition, lineColor);
	}
}

void Object3d::SetModel(const std::string& filePath)
{
	// モデルを検索してセット
	model = ModelManager::GetInstance()->FindModel(filePath);
}

void Object3d::SetShininess(float shininess)
{
	materialData->shininess = shininess;
}

void Object3d::CreateTransformationMatrix()
{
	transformationMatrixResource = obj3dCommon->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
	transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
	
	transformationMatrixData->WVP = MakeIdentity4x4();
	transformationMatrixData->World = MakeIdentity4x4();
	transformationMatrixData->WorldInverseTranspose = MakeIdentity4x4();
}

void Object3d::CreateMaterial()
{
	materialResource = obj3dCommon->GetDxCommon()->CreateBufferResource(sizeof(Material));
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->enableLighting = true;
	materialData->uvTransform = MakeIdentity4x4();
	materialData->shininess = 20.0f;
}
