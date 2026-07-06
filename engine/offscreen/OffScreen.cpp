#include "OffScreen.h"
#include "DirectXCommon.h"
#include "WinApp.h"

#include <filesystem>
#include <fstream>

#ifdef _DEBUG
#include "imgui.h"
#endif // _DEBUG

namespace Engine {

void OffScreen::Initialize() {
    dxCommon_ = DirectXCommon::GetInstance();
    srvManager_ = SrvManager::GetInstance();

    psoManager_ = std::make_unique<PipeLineManager>();
    psoManager_->Initialize(dxCommon_);

    CreatePipelines();
    CreatePingPongTextures();

    // 保存済み設定があれば復元(無ければエフェクトなし=シーンをそのまま表示)
    LoadFromJson();
}

void OffScreen::CreatePipelines() {
    for (int i = 0; i < kShaderModeCount; ++i) {
        ShaderMode mode = static_cast<ShaderMode>(i);
        rootSignatures_[i] = psoManager_->CreateRenderRootSignature(rootSignatures_[i], mode);
        pipelineStates_[i] = psoManager_->CreateRenderGraphicsPipeLine(pipelineStates_[i], rootSignatures_[i], mode);
    }
}

void OffScreen::CreatePingPongTextures() {
    // RTV用のヒープ(ピンポン2枚)
    rtvHeap_ = dxCommon_->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
    const UINT rtvSize = dxCommon_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    clearValue.Color[0] = 0.0f;
    clearValue.Color[1] = 0.0f;
    clearValue.Color[2] = 0.0f;
    clearValue.Color[3] = 1.0f;

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    D3D12_CPU_DESCRIPTOR_HANDLE heapStart = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    for (int i = 0; i < 2; ++i) {
        RenderTexture& tex = pingPong_[i];
        tex.resource = dxCommon_->CreateRenderTextureResource(
            WinApp::kClientWidth, WinApp::kClientHeight, clearValue.Format, clearValue);

        tex.rtvHandle = heapStart;
        tex.rtvHandle.ptr += static_cast<SIZE_T>(rtvSize) * i;
        dxCommon_->GetDevice()->CreateRenderTargetView(tex.resource.Get(), &rtvDesc, tex.rtvHandle);

        tex.srvIndex = srvManager_->Allocate();
        srvManager_->CreateSRVforRenderTexture(tex.srvIndex, tex.resource.Get());
        tex.srvGpuHandle = srvManager_->GetGPUDescriptorHandle(tex.srvIndex);
    }
}

void OffScreen::Barrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    dxCommon_->GetCommandList()->ResourceBarrier(1, &barrier);
}

void OffScreen::DrawPass(IPostEffect* effect, D3D12_GPU_DESCRIPTOR_HANDLE srcSrv) {
    const int mode = static_cast<int>(effect->GetShaderMode());
    auto commandList = dxCommon_->GetCommandList();

    psoManager_->DrawCommonSetting(pipelineStates_[mode], rootSignatures_[mode]);
    // ルートパラメータ0 = ソーステクスチャ(全オフスクリーン用RootSignature共通)
    commandList->SetGraphicsRootDescriptorTable(0, srcSrv);
    // 各エフェクト固有のリソース(CBV / 深度SRV 等)
    effect->BindExtra(commandList.Get());
    commandList->DrawInstanced(3, 1, 0, 0);
}

void OffScreen::Draw() {
    auto commandList = dxCommon_->GetCommandList();

    // 有効なエフェクトのみを適用順に収集
    std::vector<IPostEffect*> active;
    active.reserve(effects_.size());
    for (auto& effect : effects_) {
        if (effect->IsEnabled()) {
            active.push_back(effect.get());
        }
    }

    // エフェクト側の毎フレーム更新
    for (IPostEffect* effect : active) {
        effect->SetProjection(projection_);
        effect->Update();
    }

    // エフェクトが無ければコピー(kNone)でシーンをそのままバックバッファへ。
    // バックバッファは PreDraw でバインド済みのため再バインド不要。
    if (active.empty()) {
        const int mode = static_cast<int>(ShaderMode::kNone);
        psoManager_->DrawCommonSetting(pipelineStates_[mode], rootSignatures_[mode]);
        commandList->SetGraphicsRootDescriptorTable(0, dxCommon_->GetOffScreenGPUHandle());
        commandList->DrawInstanced(3, 1, 0, 0);
        return;
    }

    // ピンポンでエフェクトを鎖状に適用する。
    // 最初の入力はシーンテクスチャ、最後の出力はバックバッファ。
    D3D12_GPU_DESCRIPTOR_HANDLE srcSrv = dxCommon_->GetOffScreenGPUHandle();
    int writeIndex = 0;
    const size_t count = active.size();
    for (size_t i = 0; i < count; ++i) {
        const bool isLast = (i + 1 == count);

        if (isLast) {
            // 最終パスはバックバッファへ描画(PreDraw と同じ RTV/DSV をバインド)
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCommon_->GetCurrentBackBufferRTVHandle();
            D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxCommon_->GetDSVCPUDescriptorHandle(0);
            commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
        } else {
            // 中間パスはピンポンテクスチャへ描画
            RenderTexture& dst = pingPong_[writeIndex];
            Barrier(dst.resource.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
            commandList->OMSetRenderTargets(1, &dst.rtvHandle, false, nullptr);
        }

        DrawPass(active[i], srcSrv);

        if (!isLast) {
            // 書き込んだテクスチャを次パスの入力にするため読み取り状態へ戻す
            RenderTexture& dst = pingPong_[writeIndex];
            Barrier(dst.resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
            srcSrv = dst.srvGpuHandle;
            writeIndex = 1 - writeIndex;
        }
    }
}

void OffScreen::AddEffect(const std::string& typeName) {
    std::unique_ptr<IPostEffect> effect = CreatePostEffect(typeName);
    if (!effect) {
        return;
    }
    effect->Initialize(dxCommon_);
    effects_.push_back(std::move(effect));
}

void OffScreen::SaveToJson() {
    nlohmann::json root;
    root["effects"] = nlohmann::json::array();
    for (auto& effect : effects_) {
        nlohmann::json entry;
        entry["type"] = effect->GetTypeName();
        entry["enabled"] = effect->IsEnabled();
        nlohmann::json params;
        effect->ToJson(params);
        entry["params"] = params;
        root["effects"].push_back(entry);
    }

    std::filesystem::path path = kJsonPath;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream ofs(path);
    if (ofs) {
        ofs << root.dump(4);
    }
}

void OffScreen::LoadFromJson() {
    std::ifstream ifs(kJsonPath);
    if (!ifs) {
        return;
    }

    nlohmann::json root;
    try {
        ifs >> root;
    } catch (...) {
        return;
    }

    if (!root.contains("effects") || !root["effects"].is_array()) {
        return;
    }

    effects_.clear();
    for (const auto& entry : root["effects"]) {
        const std::string type = entry.value("type", std::string());
        std::unique_ptr<IPostEffect> effect = CreatePostEffect(type);
        if (!effect) {
            continue;
        }
        effect->Initialize(dxCommon_);
        effect->SetEnabled(entry.value("enabled", true));
        if (entry.contains("params")) {
            effect->FromJson(entry["params"]);
        }
        effects_.push_back(std::move(effect));
    }
}

void OffScreen::DrawCommonSetting() {
#ifdef _DEBUG
    ImGui::Begin("OffScreen");

    // --- エフェクト追加 / 保存・読み込み ---
    const std::vector<std::string>& types = GetPostEffectTypeNames();
    if (addTypeIndex_ >= static_cast<int>(types.size())) {
        addTypeIndex_ = 0;
    }
    if (ImGui::BeginCombo("##addType", types[addTypeIndex_].c_str())) {
        for (int i = 0; i < static_cast<int>(types.size()); ++i) {
            const bool selected = (addTypeIndex_ == i);
            if (ImGui::Selectable(types[i].c_str(), selected)) {
                addTypeIndex_ = i;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Add")) {
        AddEffect(types[addTypeIndex_]);
    }
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        SaveToJson();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        LoadFromJson();
    }

    ImGui::Separator();
    ImGui::Text("Layers (top = applied first)");

    // --- 各レイヤーの表示・操作 ---
    int moveUp = -1;
    int moveDown = -1;
    int removeAt = -1;
    for (int i = 0; i < static_cast<int>(effects_.size()); ++i) {
        IPostEffect* effect = effects_[i].get();
        ImGui::PushID(i);

        ImGui::Checkbox("##enabled", effect->EnabledPtr());
        ImGui::SameLine();

        const bool open = ImGui::TreeNode("layer", "%d: %s", i, effect->GetTypeName());
        ImGui::SameLine();
        if (ImGui::SmallButton("Up")) {
            moveUp = i;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Down")) {
            moveDown = i;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("X")) {
            removeAt = i;
        }
        if (open) {
            effect->DrawImGui();
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    // --- 並び替え / 削除の反映(ループ後にまとめて処理) ---
    if (moveUp > 0) {
        std::swap(effects_[moveUp], effects_[moveUp - 1]);
    }
    if (moveDown >= 0 && moveDown + 1 < static_cast<int>(effects_.size())) {
        std::swap(effects_[moveDown], effects_[moveDown + 1]);
    }
    if (removeAt >= 0 && removeAt < static_cast<int>(effects_.size())) {
        effects_.erase(effects_.begin() + removeAt);
    }

    ImGui::End();
#endif // _DEBUG
}

} // namespace Engine
