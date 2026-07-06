#include "ParticleDebugScene.h"
#include "SceneManager.h"
#include "ImGuiManager.h"
#include <imgui.h>
#include "GlobalVariables.h"
#include "line/DrawLine3D.h"

using namespace Engine;
void ParticleDebugScene::Initialize() {
    audio_ = Audio::GetInstance();
    objCommon_ = Object3dCommon::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.Initialize();
    
    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);
    debugCamera_->SetActive(false);

    skybox_ = std::make_unique<Skybox>();
    skybox_->Initialize("skybox.dds");

    emitter_ = std::make_unique<ParticleEmitter>();
    emitter_->Initialize(particleName_, modelPath_);
}

void ParticleDebugScene::Finalize() {}

void ParticleDebugScene::Update() {
#ifdef _DEBUG
    Debug();
#endif
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }

    skybox_->Update(vp_);
    if (emitter_) {
        emitter_->Update(vp_);
    }

    if (input_->TriggerKey(DIK_ESCAPE)) {
        sceneManager_->NextSceneReservation("TITLE");
    }
}

void ParticleDebugScene::Draw() {
    skybox_->Draw();

    ptCommon_->DrawCommonSetting();
    if (emitter_) {
        emitter_->Draw(static_cast<PrimitiveType>(currentPrimitive_));
        emitter_->DrawEmitter();
    }

    DrawLine3D::GetInstance()->Draw(vp_);
}

void ParticleDebugScene::DrawForOffScreen() {}

void ParticleDebugScene::Debug() {
    ImGui::Begin("Particle Editor Scene");
    
    if (ImGui::Button("Back to Title")) {
        sceneManager_->NextSceneReservation("TITLE");
    }

    ImGui::Separator();
    
    ImGui::InputText("Particle Name", particleName_, sizeof(particleName_));
    ImGui::InputText("Model Path", modelPath_, sizeof(modelPath_));
    
    const char* primitiveNames[] = { "Normal", "Ring", "Cylinder" };
    ImGui::Combo("Primitive Type", &currentPrimitive_, primitiveNames, IM_ARRAYSIZE(primitiveNames));

    if (ImGui::Button("Reset Emitter")) {
        emitter_ = std::make_unique<ParticleEmitter>();
        emitter_->Initialize(particleName_, modelPath_);
    }

    if (emitter_) {
        if (ImGui::Button("Save to JSON")) {
            GlobalVariables::GetInstance()->SaveFile(particleName_);
            std::string message = std::format("{}.json saved.", particleName_);
            MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
        }
    }

    ImGui::End();

    if (emitter_) {
        emitter_->imgui();
    }
}
