#pragma once
#include "DirectXCommon.h"
#include "ViewProjection.h"
#include <Matrix4x4.h>
#include <Vector4.h>

/// <summary>
/// スカイボックス用変換データ管理クラス
/// </summary>
class SkyboxTransform {
  public:
    /// <summary>
    /// マテリアル構造体
    /// </summary>
    struct Material {
        Vector4 color;
        Matrix4x4 uvTransform;
        float padding[3];
    };

    /// <summary>
    /// 変換行列構造体
    /// </summary>
    struct TransformationMatrix {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 WorldInverseTranspose;
    };

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(DirectXCommon *dxCommon);

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update(const ViewProjection &viewProjection, float scale);

    /// <summary>
    /// マテリアルリソースの取得
    /// </summary>
    ID3D12Resource *GetMaterialResource() const { return materialResource_.Get(); }

    /// <summary>
    /// 変換行列リソースの取得
    /// </summary>
    ID3D12Resource *GetTransformationMatrixResource() const { return transformationMatrixResource_.Get(); }

  private:
    /// <summary>
    /// マテリアル作成
    /// </summary>
    void CreateMaterial();

    /// <summary>
    /// 変換行列作成
    /// </summary>
    void CreateTransformationMatrix();

    DirectXCommon *dxCommon_ = nullptr;

    // --- マテリアルデータ ---
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;
    Material *materialData_ = nullptr;

    // --- 変換行列データ ---
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_ = nullptr;
    TransformationMatrix *transformationMatrixData_ = nullptr;
};