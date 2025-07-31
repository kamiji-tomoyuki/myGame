#include "Framework.h"
#include "GlobalVariables.h"
#include "ImGuiManager.h"
#include "engine/Frame/Frame.h"
#include <D3DResourceLeakChecker.h>

void Framework::Run() {
    // ゲームの初期化
    Initialize();

    while (true) // ゲームループ
    {
        // 更新
        Update();
        // 終了リクエストが来たら抜ける
        if (IsEndRequest()) {
            break;
        }
        // 描画
        Draw();
    }
    // ゲームの終了
    Finalize();
}

void Framework::Initialize() {

    D3DResourceLeakChecker();

    ///---------WinApp--------
    // WindowsAPIの初期化
    winApp = WinApp::GetInstance();
    winApp->Initialize();
    ///-----------------------

    ///---------DirectXCommon----------
    // DirectXCommonの初期化
    dxCommon = DirectXCommon::GetInstance();
    dxCommon->Initialize(winApp);
    ///--------------------------------

    /// ---------ImGui---------
#ifdef _DEBUG
    ImGuiManager::GetInstance()->Initialize(winApp);
#endif // _DEBUG
       /// -----------------------

    ///--------SRVManager--------
    // SRVマネージャの初期化
    srvManager = SrvManager::GetInstance();
    srvManager->Initialize();
    ///--------------------------

    // offscreenのSRV作成
    dxCommon->CreateOffscreenSRV();
    // depthのSRV作成
    dxCommon->CreateDepthSRV();

    ///----------Input-----------
    // 入力の初期化
    input = Input::GetInstance();
    input->Init(winApp->GetHInstance(), winApp->GetHwnd());
    ///--------------------------

    ///-----------TextureManager----------
    textureManager_ = TextureManager::GetInstance();
    textureManager_->Initialize(srvManager);
    ///-----------------------------------

    ///-----------ModelManager------------
    modelManager_ = ModelManager::GetInstance();
    modelManager_->Initialize(srvManager);
    ///----------------------------------

    ///----------SpriteCommon------------
    // スプライト共通部の初期化
    spriteCommon = SpriteCommon::GetInstance();
    spriteCommon->Initialize();
    ///----------------------------------

    ///----------Object3dCommon-----------
    // 3Dオブジェクト共通部の初期化
    object3dCommon = Object3dCommon::GetInstance();
    object3dCommon->Initialize();
    ///-----------------------------------

    ///----------AnimationManager-----------
    // 3Dオブジェクト共通部の初期化
    animationManager_ = AnimationManager::GetInstance();
    animationManager_->Initialize();
    ///-----------------------------------

    ///----------ParticleCommon------------
    particleCommon = ParticleCommon::GetInstance();
    particleCommon->Initialize(dxCommon);
    ///------------------------------------

    ///----------SkyboxManager------------
    skyboxManager_ = SkyboxManager::GetInstance();
    skyboxManager_->Initialize();
    ///------------------------------------

    ///---------Audio-------------
    audio = Audio::GetInstance();
    audio->Initialize();
    ///---------------------------

    ///-------CollisionManager--------------
    collisionManager_ = std::make_unique<CollisionManager>();
    collisionManager_->Initialize();
    ///-------------------------------------

    ///-------SceneManager--------
    sceneManager_ = SceneManager::GetInstance();
    sceneManager_->Initialize();
    ///---------------------------

    ///-------OffScreen--------
    offscreen_ = std::make_unique<OffScreen>();
    offscreen_->Initialize();
    ///------------------------

    ///-------DrawLine3D-------
    line3d_ = DrawLine3D::GetInstance();
    line3d_->Initialize();
    ///------------------------

    LightGroup::GetInstance()->Initialize();

    GlobalVariables::GetInstance()->LoadFiles();

    /// 時間の初期化
    Frame::Init();
}

void Framework::Finalize() {
    sceneManager_->Finalize();

    // WindowsAPIの終了処理
    winApp->Finalize();

    /// -------TextureManager-------
    textureManager_->Finalize();
    ///-----------------------------

    /// -------ModelCommon-------
    animationManager_->Finalize();
    ///---------------------------

    /// -------ModelCommon-------
    modelManager_->Finalize();
    ///---------------------------

#ifdef _DEBUG
    ImGuiManager::GetInstance()->Finalize();
#endif // _DEBUG
    line3d_->Finalize();
    srvManager->Finalize();
    audio->Finalize();
    LightGroup::GetInstance()->Finalize();
    object3dCommon->Finalize();
    spriteCommon->Finalize();
    particleCommon->Finalize();
    skyboxManager_->Finalize();
    dxCommon->Finalize();
    delete sceneFactory_;
}

void Framework::Update() {
    /// deltaTimeの更新
    Frame::Update();
#ifdef _DEBUG
    ImGuiManager::GetInstance()->Begin();
    GlobalVariables::GetInstance()->Update();
#endif // _DEBUG
    offscreen_->DrawCommonSetting();
    sceneManager_->Update();
    collisionManager_->Update();
#ifdef _DEBUG
    DisplayFPS();
    ImGuiManager::GetInstance()->End();
#endif // _DEBUG

    /// -------更新処理開始----------

    // -------Input-------
    // 入力の更新
    input->Update();
    // -------------------

    /// -------更新処理終了----------
    endRequest_ = winApp->ProcessMessage();
}

void Framework::LoadResource() {
}

void Framework::PlaySounds() {
}

void Framework::Draw() {
}

void Framework::DisplayFPS() {
#ifdef _DEBUG
    if (ImGui::CollapsingHeader("FPS")) {

        ImGuiIO &io = ImGui::GetIO();
        // FPSを取得
        float fps = Frame::GetFPS();
        float deltaTime = Frame::DeltaTime() * 1000.0f; // ミリ秒単位に変換

        // FPSを色付きで表示
        ImVec4 color;
        if (fps >= 59.0f) {
            color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // 60FPS付近なら緑色
        } else if (fps >= 30.0f) {
            color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // 30-59FPSなら黄色
        } else {
            color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // 30FPS未満なら赤色
        }

        ImGui::TextColored(color, "FPS: %.1f", fps);
        ImGui::TextColored(color, "Frame: %.2f ms", deltaTime);
        // ImGui::TreePop();
    }
#endif // _DEBUG
}
