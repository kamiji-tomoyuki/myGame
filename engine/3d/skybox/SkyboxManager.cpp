#include "SkyboxManager.h"

SkyboxManager *SkyboxManager::instance_ = nullptr;

SkyboxManager *SkyboxManager::GetInstance() {
    if (instance_ == nullptr) {
        instance_ = new SkyboxManager();
    }
    return instance_;
}

void SkyboxManager::Finalize() {
    delete instance_;
    instance_ = nullptr;
}

void SkyboxManager::Initialize() {
    dxCommon_ = DirectXCommon::GetInstance();
    srvManager_ = SrvManager::GetInstance();

    psoManager_ = std::make_unique<PipeLineManager>();
    psoManager_->Initialize(dxCommon_);

    // スカイボックス用のルートシグネチャとパイプライン作成
    rootSignature_ = psoManager_->CreateSkyboxRootSignature(rootSignature_);

    graphicsPipelineState_[0] = psoManager_->CreateSkyboxGraphicsPipeLine(graphicsPipelineState_[0], rootSignature_, BlendMode::kNormal);
    graphicsPipelineState_[1] = psoManager_->CreateSkyboxGraphicsPipeLine(graphicsPipelineState_[1], rootSignature_, BlendMode::kAdd);
    graphicsPipelineState_[2] = psoManager_->CreateSkyboxGraphicsPipeLine(graphicsPipelineState_[2], rootSignature_, BlendMode::kSubtract);
    graphicsPipelineState_[3] = psoManager_->CreateSkyboxGraphicsPipeLine(graphicsPipelineState_[3], rootSignature_, BlendMode::kMultiply);
    graphicsPipelineState_[4] = psoManager_->CreateSkyboxGraphicsPipeLine(graphicsPipelineState_[4], rootSignature_, BlendMode::kScreen);
}

void SkyboxManager::DrawCommonSetting() {
    psoManager_->DrawCommonSetting(graphicsPipelineState_[0], rootSignature_);
}

void SkyboxManager::SetBlendMode(BlendMode blendMode) {
    switch (blendMode) {
    case BlendMode::kNormal:
        psoManager_->DrawCommonSetting(graphicsPipelineState_[0], rootSignature_);
        break;
    case BlendMode::kAdd:
        psoManager_->DrawCommonSetting(graphicsPipelineState_[1], rootSignature_);
        break;
    case BlendMode::kSubtract:
        psoManager_->DrawCommonSetting(graphicsPipelineState_[2], rootSignature_);
        break;
    case BlendMode::kMultiply:
        psoManager_->DrawCommonSetting(graphicsPipelineState_[3], rootSignature_);
        break;
    case BlendMode::kScreen:
        psoManager_->DrawCommonSetting(graphicsPipelineState_[4], rootSignature_);
        break;
    default:
        psoManager_->DrawCommonSetting(graphicsPipelineState_[0], rootSignature_);
        break;
    }
}