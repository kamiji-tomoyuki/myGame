#pragma once
#include "DirectXCommon.h"
#include <Vector2.h>
#include <Vector4.h>
#include <vector>

/// <summary>
/// スカイボックス用ジオメトリ管理クラス
/// </summary>
class SkyboxGeometry {
  public:
    /// <summary>
    /// 頂点データ構造体
    /// </summary>
    struct VertexData {
        Vector4 position;
        Vector2 texcoord;
    };

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(DirectXCommon *dxCommon);

    /// <summary>
    /// 頂点バッファビューの取得
    /// </summary>
    const D3D12_VERTEX_BUFFER_VIEW &GetVertexBufferView() const { return vertexBufferView_; }

    /// <summary>
    /// インデックスバッファビューの取得
    /// </summary>
    const D3D12_INDEX_BUFFER_VIEW &GetIndexBufferView() const { return indexBufferView_; }

    /// <summary>
    /// インデックス数の取得
    /// </summary>
    size_t GetIndexCount() const { return indices_.size(); }

  private:
    /// <summary>
    /// 頂点データの作成
    /// </summary>
    void CreateVertexData();

    /// <summary>
    /// インデックスデータの作成
    /// </summary>
    void CreateIndexData();

    DirectXCommon *dxCommon_ = nullptr;

    // --- 頂点データ ---
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    VertexData *vertexData_ = nullptr;

    // --- インデックスデータ ---
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
    uint32_t *indexData_ = nullptr;
    std::vector<uint32_t> indices_;

    // --- UV座標（各面用） ---
    Vector2 uvs_[4] = {
        {0.0f, 0.0f}, // 左上
        {0.0f, 1.0f}, // 左下
        {1.0f, 1.0f}, // 右下
        {1.0f, 0.0f}  // 右上
    };
};