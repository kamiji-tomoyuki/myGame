#include "PostEffect.h"
#include "DirectXCommon.h"
#include "myMath.h"

#ifdef _DEBUG
#include "imgui.h"
#endif // _DEBUG

namespace Engine {

// =============================================================
// Vignette
// =============================================================
void VignetteEffect::Initialize(DirectXCommon* dxCommon) {
    IPostEffect::Initialize(dxCommon);
    resource_ = dxCommon_->CreateBufferResource(sizeof(Parameter));
    resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));
    data_->strength = 1.0f;
    data_->radius = 1.0f;
    data_->exponent = 1.0f;
    data_->padding = 0.0f;
    data_->center = { 0.5f, 0.5f };
}

void VignetteEffect::BindExtra(ID3D12GraphicsCommandList* commandList) {
    commandList->SetGraphicsRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
}

void VignetteEffect::DrawImGui() {
#ifdef _DEBUG
    ImGui::DragFloat("Exponent", &data_->exponent, 0.1f, 0.0f, 10.0f);
    ImGui::DragFloat("Radius", &data_->radius, 0.01f, 0.0f, 10.0f);
    ImGui::DragFloat("Strength", &data_->strength, 0.01f);
    ImGui::DragFloat2("Center", &data_->center.x, 0.01f, -10.0f, 10.0f);
#endif // _DEBUG
}

void VignetteEffect::ToJson(nlohmann::json& j) const {
    j["strength"] = data_->strength;
    j["radius"] = data_->radius;
    j["exponent"] = data_->exponent;
    j["center"] = { data_->center.x, data_->center.y };
}

void VignetteEffect::FromJson(const nlohmann::json& j) {
    data_->strength = j.value("strength", 1.0f);
    data_->radius = j.value("radius", 1.0f);
    data_->exponent = j.value("exponent", 1.0f);
    if (j.contains("center") && j["center"].is_array() && j["center"].size() == 2) {
        data_->center = { j["center"][0].get<float>(), j["center"][1].get<float>() };
    }
}

// =============================================================
// Smooth (BoxFilter)
// =============================================================
void SmoothEffect::Initialize(DirectXCommon* dxCommon) {
    IPostEffect::Initialize(dxCommon);
    resource_ = dxCommon_->CreateBufferResource(sizeof(Parameter));
    resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));
    data_->kernelSize = 3;
}

void SmoothEffect::BindExtra(ID3D12GraphicsCommandList* commandList) {
    commandList->SetGraphicsRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
}

void SmoothEffect::DrawImGui() {
#ifdef _DEBUG
    ImGui::DragInt("Kernel Size", &data_->kernelSize, 2, 3, 7);
#endif // _DEBUG
}

void SmoothEffect::ToJson(nlohmann::json& j) const {
    j["kernelSize"] = data_->kernelSize;
}

void SmoothEffect::FromJson(const nlohmann::json& j) {
    data_->kernelSize = j.value("kernelSize", 3);
}

// =============================================================
// Gauss (GaussianFilter)
// =============================================================
void GaussEffect::Initialize(DirectXCommon* dxCommon) {
    IPostEffect::Initialize(dxCommon);
    resource_ = dxCommon_->CreateBufferResource(sizeof(Parameter));
    resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));
    data_->kernelSize = 3;
    data_->sigma = 1.0f;
}

void GaussEffect::BindExtra(ID3D12GraphicsCommandList* commandList) {
    commandList->SetGraphicsRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
}

void GaussEffect::DrawImGui() {
#ifdef _DEBUG
    ImGui::DragInt("Kernel Size", &data_->kernelSize, 2, 3, 7);
    ImGui::DragFloat("Sigma", &data_->sigma, 0.01f, 0.01f, 10.0f);
#endif // _DEBUG
}

void GaussEffect::ToJson(nlohmann::json& j) const {
    j["kernelSize"] = data_->kernelSize;
    j["sigma"] = data_->sigma;
}

void GaussEffect::FromJson(const nlohmann::json& j) {
    data_->kernelSize = j.value("kernelSize", 3);
    data_->sigma = j.value("sigma", 1.0f);
}

// =============================================================
// DepthOutline
// =============================================================
void DepthOutlineEffect::Initialize(DirectXCommon* dxCommon) {
    IPostEffect::Initialize(dxCommon);
    resource_ = dxCommon_->CreateBufferResource(sizeof(Parameter));
    resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));
    data_->projectionInverse = MakeIdentity4x4();
    data_->kernelSize = 3;
    projection_ = MakeIdentity4x4();
}

void DepthOutlineEffect::Update() {
    // プロジェクション行列の逆行列を毎フレーム反映
    data_->projectionInverse = Inverse(projection_);
}

void DepthOutlineEffect::BindExtra(ID3D12GraphicsCommandList* commandList) {
    commandList->SetGraphicsRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
    // 深度テクスチャ(t1)をルートパラメータ2にバインド
    commandList->SetGraphicsRootDescriptorTable(2, dxCommon_->GetDepthGPUHandle());
}

void DepthOutlineEffect::DrawImGui() {
#ifdef _DEBUG
    ImGui::DragInt("Kernel Size", &data_->kernelSize, 2, 3, 7);
#endif // _DEBUG
}

void DepthOutlineEffect::ToJson(nlohmann::json& j) const {
    j["kernelSize"] = data_->kernelSize;
}

void DepthOutlineEffect::FromJson(const nlohmann::json& j) {
    data_->kernelSize = j.value("kernelSize", 3);
}

// =============================================================
// RadialBlur
// =============================================================
void RadialBlurEffect::Initialize(DirectXCommon* dxCommon) {
    IPostEffect::Initialize(dxCommon);
    resource_ = dxCommon_->CreateBufferResource(sizeof(Parameter));
    resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));
    data_->center = { 0.5f, 0.5f };
    data_->blurWidth = 0.01f;
}

void RadialBlurEffect::BindExtra(ID3D12GraphicsCommandList* commandList) {
    commandList->SetGraphicsRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
}

void RadialBlurEffect::DrawImGui() {
#ifdef _DEBUG
    ImGui::DragFloat2("Center", &data_->center.x, 0.1f);
    ImGui::DragFloat("Width", &data_->blurWidth, 0.01f);
#endif // _DEBUG
}

void RadialBlurEffect::ToJson(nlohmann::json& j) const {
    j["center"] = { data_->center.x, data_->center.y };
    j["blurWidth"] = data_->blurWidth;
}

void RadialBlurEffect::FromJson(const nlohmann::json& j) {
    if (j.contains("center") && j["center"].is_array() && j["center"].size() == 2) {
        data_->center = { j["center"][0].get<float>(), j["center"][1].get<float>() };
    }
    data_->blurWidth = j.value("blurWidth", 0.01f);
}

// =============================================================
// Factory
// =============================================================
const std::vector<std::string>& GetPostEffectTypeNames() {
    static const std::vector<std::string> kTypeNames = {
        "Grayscale",
        "Vignette",
        "Smooth",
        "Gauss",
        "Outline",
        "DepthOutline",
        "RadialBlur",
    };
    return kTypeNames;
}

std::unique_ptr<IPostEffect> CreatePostEffect(const std::string& typeName) {
    if (typeName == "Grayscale") { return std::make_unique<GrayscaleEffect>(); }
    if (typeName == "Vignette") { return std::make_unique<VignetteEffect>(); }
    if (typeName == "Smooth") { return std::make_unique<SmoothEffect>(); }
    if (typeName == "Gauss") { return std::make_unique<GaussEffect>(); }
    if (typeName == "Outline") { return std::make_unique<OutlineEffect>(); }
    if (typeName == "DepthOutline") { return std::make_unique<DepthOutlineEffect>(); }
    if (typeName == "RadialBlur") { return std::make_unique<RadialBlurEffect>(); }
    return nullptr;
}

} // namespace Engine
