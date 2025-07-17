#include "Skybox.h"
#include <cassert>

void Skybox::Initialize(const std::string &textureFilePath) {
    // 管理クラスの取得
    skyboxManager_ = SkyboxManager::GetInstance();

    // 構成要素の初期化
    geometry_ = std::make_unique<SkyboxGeometry>();
    geometry_->Initialize(skyboxManager_->GetDxCommon());

    transform_ = std::make_unique<SkyboxTransform>();
    transform_->Initialize(skyboxManager_->GetDxCommon());

    // テクスチャファイルパスの設定
    textureFilePath_ = "resources/images/" + textureFilePath;

    // テクスチャの読み込み
    TextureManager::GetInstance()->LoadTexture(textureFilePath_);
    textureIndex_ = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath_);
}

void Skybox::Update(const ViewProjection &viewProjection) {
    // 変換データの更新
    transform_->Update(viewProjection, scale_);
}

void Skybox::Draw() {
    // ブレンドモード設定
    skyboxManager_->SetBlendMode(blendMode_);

    auto commandList = skyboxManager_->GetDxCommon()->GetCommandList();

    // 定数バッファの設定
    commandList->SetGraphicsRootConstantBufferView(0, transform_->GetMaterialResource()->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, transform_->GetTransformationMatrixResource()->GetGPUVirtualAddress());

    // 頂点バッファの設定
    commandList->IASetVertexBuffers(0, 1, &geometry_->GetVertexBufferView());
    commandList->IASetIndexBuffer(&geometry_->GetIndexBufferView());

    // テクスチャの設定
    skyboxManager_->GetSrvManager()->SetGraphicsRootDescriptorTable(2, textureIndex_);

    // プリミティブトポロジーの設定
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 描画
    commandList->DrawIndexedInstanced(static_cast<UINT>(geometry_->GetIndexCount()), 1, 0, 0, 0);
}

void Skybox::SetBlendMode(BlendMode blendMode) {
    blendMode_ = blendMode;
}