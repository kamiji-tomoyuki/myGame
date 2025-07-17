#pragma once
#include "DirectXCommon.h"
#include "PipeLineManager.h"
#include "SrvManager.h"
#include <memory>

/// <summary>
/// スカイボックス描画の共通設定を管理するクラス
/// </summary>
class SkyboxManager {
  public:
    /// <summary>
    /// インスタンス取得
    /// </summary>
    static SkyboxManager *GetInstance();

    /// <summary>
    /// 終了処理
    /// </summary>
    static void Finalize();

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 描画共通設定
    /// </summary>
    void DrawCommonSetting();

    /// <summary>
    /// ブレンドモード設定
    /// </summary>
    void SetBlendMode(BlendMode blendMode);

    /// <summary>
    /// DirectXCommonの取得
    /// </summary>
    DirectXCommon *GetDxCommon() const { return dxCommon_; }

    /// <summary>
    /// SrvManagerの取得
    /// </summary>
    SrvManager *GetSrvManager() const { return srvManager_; }

  private:
    SkyboxManager() = default;
    ~SkyboxManager() = default;
    SkyboxManager(const SkyboxManager &) = delete;
    SkyboxManager &operator=(const SkyboxManager &) = delete;

    static SkyboxManager *instance_;

    // --- 基盤 ---
    DirectXCommon *dxCommon_ = nullptr;
    SrvManager *srvManager_ = nullptr;
    std::unique_ptr<PipeLineManager> psoManager_ = nullptr;

    // --- パイプライン ---
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_[5];
};