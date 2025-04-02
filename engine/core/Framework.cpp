#include "Framework.h"
#include "GlobalVariables.h"
#include "ImGuiManager.h"
#include <D3DResourceLeakChecker.h>
#include "engine/Frame/Frame.h"

void Framework::Run()
{
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

void Framework::Initialize()
{

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

void Framework::Finalize()
{
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
	dxCommon->Finalize();
	delete sceneFactory_;
}

void Framework::Update()
{
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

void Framework::LoadResource()
{
	
}

void  Framework::PlaySounds() {
	
}

void Framework::Draw()
{

}

void Framework::DisplayFPS()
{
#ifdef _DEBUG
	ImGuiIO& io = ImGui::GetIO();

	// ウィンドウ固定
	ImGui::SetNextWindowPos(ImVec2(1230, 0), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.0f); // 背景を完全透明に設定

	// ウィンドウフラグを設定
	ImGui::Begin("FPS Overlay", nullptr,
		ImGuiWindowFlags_NoTitleBar |         // タイトルバーを非表示
		ImGuiWindowFlags_NoResize |          // リサイズを禁止
		ImGuiWindowFlags_NoMove |            // ウィンドウの移動を禁止
		ImGuiWindowFlags_NoScrollbar |       // スクロールバーを非表示
		ImGuiWindowFlags_NoCollapse |        // 折りたたみボタンを非表示
		ImGuiWindowFlags_AlwaysAutoResize |  // 必要なサイズに自動調整
		ImGuiWindowFlags_NoBackground        // 背景を非表示
	);

	// 文字色を緑に設定
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(100, 255, 100, 255));
	ImGui::Text("%.1f", io.Framerate);
	ImGui::PopStyleColor();

	ImGui::End();
#endif // _DEBUG
}
