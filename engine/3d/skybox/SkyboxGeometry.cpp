#include "SkyboxGeometry.h"
#include <cassert>

void SkyboxGeometry::Initialize(DirectXCommon *dxCommon) {
    dxCommon_ = dxCommon;
    CreateVertexData();
    CreateIndexData();
}

void SkyboxGeometry::CreateVertexData() {
    // 頂点データの作成（立方体の内側向き）
    std::vector<VertexData> vertices;
    vertices.resize(24); // 6面 * 4頂点

    float hs = 1.0f; // ハーフサイズ

    // 右面 (+X) - 内側向きなので頂点順序を逆に
    vertices[0] = {{hs, hs, hs, 1.0f}, uvs_[0]};
    vertices[1] = {{hs, -hs, hs, 1.0f}, uvs_[3]};
    vertices[2] = {{hs, -hs, -hs, 1.0f}, uvs_[2]};
    vertices[3] = {{hs, hs, -hs, 1.0f}, uvs_[1]};

    // 左面 (-X) - 内側向きなので頂点順序を逆に
    vertices[4] = {{-hs, hs, -hs, 1.0f}, uvs_[0]};
    vertices[5] = {{-hs, -hs, -hs, 1.0f}, uvs_[3]};
    vertices[6] = {{-hs, -hs, hs, 1.0f}, uvs_[2]};
    vertices[7] = {{-hs, hs, hs, 1.0f}, uvs_[1]};

    // 前面 (+Z) - 内側向きなので頂点順序を逆に
    vertices[8] = {{-hs, hs, hs, 1.0f}, uvs_[0]};
    vertices[9] = {{-hs, -hs, hs, 1.0f}, uvs_[3]};
    vertices[10] = {{hs, -hs, hs, 1.0f}, uvs_[2]};
    vertices[11] = {{hs, hs, hs, 1.0f}, uvs_[1]};

    // 背面 (-Z) - 内側向きなので頂点順序を逆に
    vertices[12] = {{hs, hs, -hs, 1.0f}, uvs_[0]};
    vertices[13] = {{hs, -hs, -hs, 1.0f}, uvs_[3]};
    vertices[14] = {{-hs, -hs, -hs, 1.0f}, uvs_[2]};
    vertices[15] = {{-hs, hs, -hs, 1.0f}, uvs_[1]};

    // 上面 (+Y) - 内側向きなので頂点順序を逆に
    vertices[16] = {{-hs, hs, -hs, 1.0f}, uvs_[0]};
    vertices[17] = {{-hs, hs, hs, 1.0f}, uvs_[3]};
    vertices[18] = {{hs, hs, hs, 1.0f}, uvs_[2]};
    vertices[19] = {{hs, hs, -hs, 1.0f}, uvs_[1]};

    // 下面 (-Y) - 内側向きなので頂点順序を逆に
    vertices[20] = {{-hs, -hs, hs, 1.0f}, uvs_[0]};
    vertices[21] = {{-hs, -hs, -hs, 1.0f}, uvs_[3]};
    vertices[22] = {{hs, -hs, -hs, 1.0f}, uvs_[2]};
    vertices[23] = {{hs, -hs, hs, 1.0f}, uvs_[1]};

    // 頂点バッファの作成
    vertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * vertices.size());
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * vertices.size());
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    // 頂点データのマッピング
    vertexResource_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData_));
    std::memcpy(vertexData_, vertices.data(), sizeof(VertexData) * vertices.size());
}

void SkyboxGeometry::CreateIndexData() {
    // インデックスデータの作成（内側向きの面なので時計回り）
    indices_ = {
        // 右面
        0, 1, 2, 0, 2, 3,
        // 左面
        4, 5, 6, 4, 6, 7,
        // 前面
        8, 9, 10, 8, 10, 11,
        // 背面
        12, 13, 14, 12, 14, 15,
        // 上面
        16, 17, 18, 16, 18, 19,
        // 下面
        20, 21, 22, 20, 22, 23};

    // インデックスバッファの作成
    indexResource_ = dxCommon_->CreateBufferResource(sizeof(uint32_t) * indices_.size());
    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = UINT(sizeof(uint32_t) * indices_.size());
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    // インデックスデータのマッピング
    indexResource_->Map(0, nullptr, reinterpret_cast<void **>(&indexData_));
    std::memcpy(indexData_, indices_.data(), sizeof(uint32_t) * indices_.size());
}