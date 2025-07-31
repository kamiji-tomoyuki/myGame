#include "SkyboxTransform.h"
#include "myMath.h"

void SkyboxTransform::Initialize(DirectXCommon *dxCommon) {
    dxCommon_ = dxCommon;
    CreateMaterial();
    CreateTransformationMatrix();
}

void SkyboxTransform::Update(const ViewProjection &viewProjection, float scale) {
    // スカイボックスは常にカメラを中心に配置
    // 平行移動成分を除去したビュー行列を作成
    Matrix4x4 skyboxViewMatrix = viewProjection.matView_;
    skyboxViewMatrix.m[3][0] = 0.0f; // X軸の平行移動を除去
    skyboxViewMatrix.m[3][1] = 0.0f; // Y軸の平行移動を除去
    skyboxViewMatrix.m[3][2] = 0.0f; // Z軸の平行移動を除去

    // スケール行列を作成
    Matrix4x4 scaleMatrix = MakeScaleMatrix({scale, scale, scale});

    // 変換行列の計算
    Matrix4x4 worldMatrix = scaleMatrix;
    Matrix4x4 worldViewProjectionMatrix = worldMatrix * skyboxViewMatrix * viewProjection.matProjection_;

    // 変換行列データの更新
    transformationMatrixData_->WVP = worldViewProjectionMatrix;
    transformationMatrixData_->World = worldMatrix;
    transformationMatrixData_->WorldInverseTranspose = Transpose(Inverse(worldMatrix));

    // マテリアルデータの更新
    materialData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
    materialData_->uvTransform = MakeIdentity4x4();
}

void SkyboxTransform::CreateMaterial() {
    // マテリアルリソースの作成
    materialResource_ = dxCommon_->CreateBufferResource(sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void **>(&materialData_));

    // マテリアルデータの初期化
    materialData_->color = {1.0f, 1.0f, 1.0f, 1.0f};
    materialData_->uvTransform = MakeIdentity4x4();
}

void SkyboxTransform::CreateTransformationMatrix() {
    // 変換行列リソースの作成
    transformationMatrixResource_ = dxCommon_->CreateBufferResource(sizeof(TransformationMatrix));
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void **>(&transformationMatrixData_));

    // 変換行列データの初期化
    transformationMatrixData_->WVP = MakeIdentity4x4();
    transformationMatrixData_->World = MakeIdentity4x4();
    transformationMatrixData_->WorldInverseTranspose = MakeIdentity4x4();
}