#pragma once
#include "BaseScene.h"
#include "ParticleEmitter.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "Input.h"
#include "Audio.h"
#include "Object3dCommon.h"
#include "SpriteCommon.h"
#include "ParticleCommon.h"
#include "DebugCamera.h"
#include "Skybox.h"
#include <memory>
#include <string>

/// <summary>
/// パーティクル調整用デモシーン
/// </summary>
class ParticleDebugScene : public BaseScene {
public:
    void Initialize() override;
    void Finalize() override;
    void Update() override;
    void Draw() override;
    void DrawForOffScreen() override;

    ViewProjection* GetViewProjection() override { return &vp_; }

private:
    void Debug();

private:
    Audio* audio_ = nullptr;
    Object3dCommon* objCommon_ = nullptr;
    SpriteCommon* spCommon_ = nullptr;
    ParticleCommon* ptCommon_ = nullptr;
    Input* input_ = nullptr;

    ViewProjection vp_;
    std::unique_ptr<DebugCamera> debugCamera_;
    std::unique_ptr<Skybox> skybox_;

    std::unique_ptr<ParticleEmitter> emitter_;
    char particleName_[64] = "new_particle";
    char modelPath_[128] = "debug/ringPlane.obj";
    int currentPrimitive_ = 0;
};
