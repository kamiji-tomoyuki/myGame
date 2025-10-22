#include "TitleScene.h"
#include "ImGuiManager.h"
#include "SceneManager.h"
#include "SrvManager.h"

#ifdef _DEBUG
#include <imgui.h>
#endif // _DEBUG

#include <LightGroup.h>
#include <line/DrawLine3D.h>

void TitleScene::Initialize() {
    audio_ = Audio::GetInstance();
    objCommon_ = Object3dCommon::GetInstance();
    spCommon_ = SpriteCommon::GetInstance();
    ptCommon_ = ParticleCommon::GetInstance();
    input_ = Input::GetInstance();
    vp_.Initialize();
    vp_.translation_ = {0.0f, 0.0f, -10.0f};

    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize(&vp_);

    wt1_.Initialize();
    wt2_.Initialize();

    skybox_ = std::make_unique<Skybox>();
    skybox_->Initialize("skybox.dds");

    player_ = std::make_unique<TitleCharacter>();
    player_->Init();

    title2d_ = std::make_unique<Sprite>();
    title2d_->Initialize("title.png", {0.0f, 0.0f});

    space_ = std::make_unique<Sprite>();
    space_->Initialize("space.png", {160.0f, 470.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f,0.5f});

    emitter_ = std::make_unique<ParticleEmitter>();
    emitter_->Initialize("test", "debug/ringPlane.obj");

}

void TitleScene::Finalize() {
}

void TitleScene::Update() {
#ifdef _DEBUG
    // デバッグ
    Debug();
#endif // _DEBUG

    // カメラ更新
    CameraUpdate();

    // シーン切り替え
    ChangeScene();

    emitter_->Update(vp_);
    player_->Update();
   
    skybox_->Update(vp_);

    if (timer_ < 0.0f || timer_ > 1.0f) {
        add_ *= -1.0f;
    }
    timer_ += add_;
    space_->SetAlpha(Lerp(1.0f, 0.0f, timer_));

    wt1_.UpdateMatrix();
    wt2_.UpdateMatrix();
}

void TitleScene::Draw() {
    /// -------描画処理開始-------

    skybox_->Draw();

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----
    title2d_->Draw();
    space_->Draw();
    //------------------------

    objCommon_->skinningDrawCommonSetting();
    //-----アニメーションの描画開始-----
    //------------------------------

    objCommon_->DrawCommonSetting();
    //-----3DObjectの描画開始-----
    player_->Draw(vp_);
    //--------------------------

    /// Particleの描画準備
    ptCommon_->DrawCommonSetting();
    //------Particleの描画開始-------

    //-----------------------------

    //-----線描画-----
    DrawLine3D::GetInstance()->Draw(vp_);
    //---------------

    /// ----------------------------------

    /// -------描画処理終了-------
}

void TitleScene::DrawForOffScreen() {
    /// -------描画処理開始-------

    /// Spriteの描画準備
    spCommon_->DrawCommonSetting();
    //-----Spriteの描画開始-----

    //------------------------

    objCommon_->skinningDrawCommonSetting();
    //-----アニメーションの描画開始-----

    //------------------------------

    objCommon_->DrawCommonSetting();
    //-----3DObjectの描画開始-----

    //--------------------------

    /// Particleの描画準備
    ptCommon_->DrawCommonSetting();
    //------Particleの描画開始-------

    //-----------------------------

    /// ----------------------------------

    /// -------描画処理終了-------
}

void TitleScene::Debug() {
    ImGui::Begin("TitleScene:Debug");

    debugCamera_->imgui();

    LightGroup::GetInstance()->imgui();

    ImGui::Checkbox("roop", &roop);

    ImGui::End();

    emitter_->imgui();
}

void TitleScene::CameraUpdate() {
    if (debugCamera_->GetActive()) {
        debugCamera_->Update();
    } else {
        vp_.UpdateMatrix();
    }
}

void TitleScene::ChangeScene() {
    if (input_->TriggerKey(DIK_SPACE)) {
        sceneManager_->NextSceneReservation("GAME");
    }
}
