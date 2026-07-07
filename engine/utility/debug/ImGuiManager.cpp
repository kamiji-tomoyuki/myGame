#include "ImGuiManager.h"
#ifdef _DEBUG
#include "imgui.h"
#include"imgui_impl_win32.h"
#include <imgui_impl_dx12.h>
#include "SrvManager.h"

namespace Engine {

std::unique_ptr<ImGuiManager> ImGuiManager::instance = nullptr;

void ImGuiManager::Initialize(WinApp* winApp)
{
	dxCommon_ = DirectXCommon::GetInstance();
	srvManager_ = SrvManager::GetInstance();

	// ImGuiのコンテキストを生成
	ImGui::CreateContext();

	// Docking機能を有効化
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Docking機能を有効化
	io.Fonts->Clear(); // 既存のフォントをクリア

	// 日本語グリフ範囲 + アイコン用の記号範囲(矢印/幾何学模様/その他記号)をマージ
	ImFontGlyphRangesBuilder builder;
	builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());
	static const ImWchar iconRanges[] = {
		0x2190, 0x21FF, // Arrows
		0x2500, 0x25FF, // Box Drawing + Geometric Shapes
		0x2600, 0x26FF, // Miscellaneous Symbols
		0x2700, 0x27BF, // Dingbats
		0,
	};
	builder.AddRanges(iconRanges);
	static ImVector<ImWchar> glyphRanges;
	builder.BuildRanges(&glyphRanges);

	io.Fonts->AddFontFromFileTTF("resources/fonts/PixelMplus12-Regular.ttf", 14.0f, nullptr, glyphRanges.Data);

	// ImGuiのスタイルを設定
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(winApp->GetHwnd());

	// DX12バックエンドの初期化(エンジン共通のSRVヒープを共有する)
	ImGui_ImplDX12_InitInfo initInfo = {};
	initInfo.Device = dxCommon_->GetDevice().Get();
	initInfo.CommandQueue = dxCommon_->GetCommandQueue().Get();
	initInfo.NumFramesInFlight = static_cast<int>(dxCommon_->GetBackBufferCount());
	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
	initInfo.SrvDescriptorHeap = srvManager_->GetDescriptorHeap();
	initInfo.SrvDescriptorAllocFn = &ImGuiManager::SrvDescriptorAlloc;
	initInfo.SrvDescriptorFreeFn = &ImGuiManager::SrvDescriptorFree;
	ImGui_ImplDX12_Init(&initInfo);
}

void ImGuiManager::SrvDescriptorAlloc(ImGui_ImplDX12_InitInfo* /*info*/, D3D12_CPU_DESCRIPTOR_HANDLE* outCpu, D3D12_GPU_DESCRIPTOR_HANDLE* outGpu)
{
	SrvManager* srv = SrvManager::GetInstance();
	const uint32_t index = srv->Allocate();
	*outCpu = srv->GetCPUDescriptorHandle(index);
	*outGpu = srv->GetGPUDescriptorHandle(index);
}

void ImGuiManager::SrvDescriptorFree(ImGui_ImplDX12_InitInfo* /*info*/, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE /*gpu*/)
{
	SrvManager::GetInstance()->FreeByCPUHandle(cpu);
}

ImGuiManager* ImGuiManager::GetInstance()
{
	if (instance == nullptr) {
		instance = std::unique_ptr<ImGuiManager>(new ImGuiManager);
	}
	return instance.get();
}

void ImGuiManager::Finalize()
{
	// 後始末
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	instance.reset();
}

void ImGuiManager::Begin()
{
	// ImGuiフレーム開始
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void ImGuiManager::End()
{
	// 描画前準備
	ImGui::Render();
}

void ImGuiManager::Draw()
{
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList().Get();

	// エンジン共通のSRVヒープをセット(ImGuiのフォント・画像もここに存在する)
	ID3D12DescriptorHeap* ppHeaps[] = { srvManager_->GetDescriptorHeap() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	// 描画コマンドを発行
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

} // namespace Engine

#endif //_DEBUG
