#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include <string>
#include <vector>

#include <json.hpp>

#include <PipeLineManager.h> // ShaderMode
#include <Vector2.h>
#include <Matrix4x4.h>

namespace Engine {
class DirectXCommon;

/// <summary>
/// スクリーン演出(ポストエフェクト)1つ分を表す抽象基底クラス。
/// OffScreen が複数保持し、レイヤー順に重ね掛けする。
/// </summary>
class IPostEffect {
public:
    virtual ~IPostEffect() = default;

    /// <summary>種別名(ImGui表示 / JSON識別子)</summary>
    virtual const char* GetTypeName() const = 0;

    /// <summary>使用するシェーダーモード(PSO選択に使用)</summary>
    virtual ShaderMode GetShaderMode() const = 0;

    /// <summary>定数バッファ等の生成</summary>
    virtual void Initialize(DirectXCommon* dxCommon) { dxCommon_ = dxCommon; }

    /// <summary>毎フレーム更新(既定は何もしない)</summary>
    virtual void Update() {}

    /// <summary>
    /// 描画時の追加リソースバインド。
    /// ソーステクスチャのSRVはルートパラメータ0に呼び出し側がバインド済み。
    /// </summary>
    virtual void BindExtra(ID3D12GraphicsCommandList*) {}

    /// <summary>ImGuiでのパラメータ調整(既定は何もしない)</summary>
    virtual void DrawImGui() {}

    /// <summary>プロジェクション行列の受け取り(深度系エフェクトが使用)</summary>
    virtual void SetProjection(const Matrix4x4&) {}

    /// <summary>パラメータをJSONへ書き出し</summary>
    virtual void ToJson(nlohmann::json&) const {}

    /// <summary>パラメータをJSONから読み込み</summary>
    virtual void FromJson(const nlohmann::json&) {}

    bool IsEnabled() const { return enabled_; }
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool* EnabledPtr() { return &enabled_; }

protected:
    DirectXCommon* dxCommon_ = nullptr;
    bool enabled_ = true;
};

/// <summary>グレースケール(パラメータなし)</summary>
class GrayscaleEffect : public IPostEffect {
public:
    const char* GetTypeName() const override { return "Grayscale"; }
    ShaderMode GetShaderMode() const override { return ShaderMode::kGray; }
};

/// <summary>輝度ベースのアウトライン(パラメータなし)</summary>
class OutlineEffect : public IPostEffect {
public:
    const char* GetTypeName() const override { return "Outline"; }
    ShaderMode GetShaderMode() const override { return ShaderMode::kOutLine; }
};

/// <summary>ビネット</summary>
class VignetteEffect : public IPostEffect {
public:
    const char* GetTypeName() const override { return "Vignette"; }
    ShaderMode GetShaderMode() const override { return ShaderMode::kVignette; }
    void Initialize(DirectXCommon* dxCommon) override;
    void BindExtra(ID3D12GraphicsCommandList* commandList) override;
    void DrawImGui() override;
    void ToJson(nlohmann::json& j) const override;
    void FromJson(const nlohmann::json& j) override;

private:
    struct Parameter {
        float strength;
        float radius;
        float exponent;
        float padding;
        Vector2 center;
    };
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Parameter* data_ = nullptr;
};

/// <summary>ボックスフィルタ(平滑化)</summary>
class SmoothEffect : public IPostEffect {
public:
    const char* GetTypeName() const override { return "Smooth"; }
    ShaderMode GetShaderMode() const override { return ShaderMode::kSmooth; }
    void Initialize(DirectXCommon* dxCommon) override;
    void BindExtra(ID3D12GraphicsCommandList* commandList) override;
    void DrawImGui() override;
    void ToJson(nlohmann::json& j) const override;
    void FromJson(const nlohmann::json& j) override;

private:
    struct Parameter {
        int kernelSize;
    };
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Parameter* data_ = nullptr;
};

/// <summary>ガウシアンフィルタ</summary>
class GaussEffect : public IPostEffect {
public:
    const char* GetTypeName() const override { return "Gauss"; }
    ShaderMode GetShaderMode() const override { return ShaderMode::kGauss; }
    void Initialize(DirectXCommon* dxCommon) override;
    void BindExtra(ID3D12GraphicsCommandList* commandList) override;
    void DrawImGui() override;
    void ToJson(nlohmann::json& j) const override;
    void FromJson(const nlohmann::json& j) override;

private:
    struct Parameter {
        int kernelSize;
        float sigma;
    };
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Parameter* data_ = nullptr;
};

/// <summary>深度ベースのアウトライン</summary>
class DepthOutlineEffect : public IPostEffect {
public:
    const char* GetTypeName() const override { return "DepthOutline"; }
    ShaderMode GetShaderMode() const override { return ShaderMode::kDepth; }
    void Initialize(DirectXCommon* dxCommon) override;
    void Update() override;
    void BindExtra(ID3D12GraphicsCommandList* commandList) override;
    void DrawImGui() override;
    void SetProjection(const Matrix4x4& projection) override { projection_ = projection; }
    void ToJson(nlohmann::json& j) const override;
    void FromJson(const nlohmann::json& j) override;

private:
    struct Parameter {
        Matrix4x4 projectionInverse;
        int kernelSize;
    };
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Parameter* data_ = nullptr;
    Matrix4x4 projection_;
};

/// <summary>ラジアルブラー</summary>
class RadialBlurEffect : public IPostEffect {
public:
    const char* GetTypeName() const override { return "RadialBlur"; }
    ShaderMode GetShaderMode() const override { return ShaderMode::kBlur; }
    void Initialize(DirectXCommon* dxCommon) override;
    void BindExtra(ID3D12GraphicsCommandList* commandList) override;
    void DrawImGui() override;
    void ToJson(nlohmann::json& j) const override;
    void FromJson(const nlohmann::json& j) override;

private:
    struct Parameter {
        Vector2 center;
        float blurWidth;
    };
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    Parameter* data_ = nullptr;
};

/// <summary>種別名からエフェクトを生成する。未知の名前なら nullptr。</summary>
std::unique_ptr<IPostEffect> CreatePostEffect(const std::string& typeName);

/// <summary>生成可能なエフェクト種別名の一覧(ImGuiの追加メニュー用)</summary>
const std::vector<std::string>& GetPostEffectTypeNames();

} // namespace Engine
