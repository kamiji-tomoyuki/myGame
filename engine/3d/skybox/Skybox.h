#pragma once
#include "SkyboxGeometry.h"
#include "SkyboxManager.h"
#include "SkyboxTransform.h"
#include "TextureManager.h"
#include "ViewProjection.h"
#include <memory>
#include <string>

/// <summary>
/// スカイボックスクラス
/// </summary>
class Skybox {
  public:
    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(const std::string &textureFilePath);

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update(const ViewProjection &viewProjection);

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw();

    /// <summary>
    /// ブレンドモード設定
    /// </summary>
    void SetBlendMode(BlendMode blendMode);

    /// <summary>
    /// スカイボックスのスケール設定
    /// </summary>
    void SetScale(float scale) { scale_ = scale; }

    /// <summary>
    /// スケール値の取得
    /// </summary>
    float GetScale() const { return scale_; }

   uint32_t GetTextureIndex() { return textureIndex_; }

  private:
    // --- 管理クラス ---
    SkyboxManager *skyboxManager_ = nullptr;

    // --- 構成要素 ---
    std::unique_ptr<SkyboxGeometry> geometry_;
    std::unique_ptr<SkyboxTransform> transform_;

    // --- テクスチャ ---
    std::string textureFilePath_;
    uint32_t textureIndex_ = 0;

    // --- 設定 ---
    float scale_ = 1000.0f; // スカイボックスのスケール（非常に大きく設定）
    BlendMode blendMode_ = BlendMode::kNormal;
};