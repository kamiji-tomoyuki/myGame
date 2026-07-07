#pragma once
#include <memory>
#include <d3d12.h>
#include"WinApp.h"
#include"DirectXCommon.h"

// ImGui DX12バックエンドの初期化情報(前方宣言・グローバルスコープ)
struct ImGui_ImplDX12_InitInfo;

/// <summary>
/// ImGui管理クラス
/// </summary>
namespace Engine {
class SrvManager;

class ImGuiManager
{
private:

	static std::unique_ptr<ImGuiManager> instance;

	ImGuiManager() = default;
	ImGuiManager(ImGuiManager&) = delete;
	ImGuiManager& operator=(ImGuiManager&) = delete;


public:
	~ImGuiManager() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(WinApp* winApp);

	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns></returns>
	static ImGuiManager* GetInstance();

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

	/// <summary>
	/// ImGui受付開始
	/// </summary>
	void Begin();

	/// <summary>
	/// ImGui受付終了
	/// </summary>
	void End();

	/// <summary>
	/// 画面への描画
	/// </summary>
	void Draw();

private:

	// ImGuiバックエンドが使うSRVデスクリプタの割り当て/解放コールバック(SrvManagerを利用)
	static void SrvDescriptorAlloc(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* outCpu, D3D12_GPU_DESCRIPTOR_HANDLE* outGpu);
	static void SrvDescriptorFree(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu);

private:

	DirectXCommon* dxCommon_ = nullptr;
	// エンジン共通のSRVヒープ(ImGuiのフォント・ユーザーテクスチャもここから確保)
	SrvManager* srvManager_ = nullptr;

};

} // namespace Engine
