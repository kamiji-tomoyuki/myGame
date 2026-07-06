#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include <string>
#include <vector>

#include "SrvManager.h"
#include <PipeLineManager.h>

#include "myMath.h"
#include <Matrix4x4.h>

#include "PostEffect.h"

namespace Engine {
class DirectXCommon;

/// <summary>
/// オフスクリーン(スクリーン演出)管理クラス。
/// 複数のポストエフェクトをレイヤー順に重ね掛けする。
/// ImGui/JSON でエフェクトの追加・並び替え・パラメータ調整・保存が行える。
/// </summary>
class OffScreen {
public:
    /// <summary>初期化</summary>
    void Initialize();

    /// <summary>描画(有効な全エフェクトをレイヤー順に適用)</summary>
    void Draw();

    /// <summary>ImGuiによるエフェクト管理UI</summary>
    void DrawCommonSetting();

    /// <summary>プロジェクション行列のセット(深度系エフェクト用)</summary>
    void SetProjection(const Matrix4x4& projectionMatrix) { projection_ = projectionMatrix; }

private:
    /// <summary>シェーダーモードごとのPSO/RootSignature生成</summary>
    void CreatePipelines();

    /// <summary>ピンポン用レンダーテクスチャの生成</summary>
    void CreatePingPongTextures();

    /// <summary>エフェクトの追加</summary>
    void AddEffect(const std::string& typeName);

    /// <summary>1パス描画(source SRV -> 現在バインド中のRTV)</summary>
    void DrawPass(IPostEffect* effect, D3D12_GPU_DESCRIPTOR_HANDLE srcSrv);

    /// <summary>リソースバリア</summary>
    void Barrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

    /// <summary>JSONへ保存</summary>
    void SaveToJson();

    /// <summary>JSONから読み込み</summary>
    void LoadFromJson();

private:
    DirectXCommon* dxCommon_ = nullptr;
    SrvManager* srvManager_ = nullptr;
    std::unique_ptr<PipeLineManager> psoManager_ = nullptr;

    // シェーダーモードごとのPSO/RootSignature(添字 = ShaderModeの値)
    static const int kShaderModeCount = 8;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatures_[kShaderModeCount];
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStates_[kShaderModeCount];

    // ピンポン用レンダーテクスチャ
    struct RenderTexture {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{};
        uint32_t srvIndex = 0;
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle{};
    };
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    RenderTexture pingPong_[2];

    // エフェクト(格納順 = レイヤー順 = 適用順)
    std::vector<std::unique_ptr<IPostEffect>> effects_;

    // 深度系エフェクトへ渡すプロジェクション行列
    Matrix4x4 projection_;

    // 保存先ファイルパス
    const std::string kJsonPath = "resources/jsons/offscreen/effects.json";

    // ImGuiの追加メニューで選択中のインデックス
    int addTypeIndex_ = 0;
};

} // namespace Engine
