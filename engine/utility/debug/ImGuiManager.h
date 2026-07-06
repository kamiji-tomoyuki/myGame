#pragma once
#include <memory>
#include"WinApp.h"
#include"DirectXCommon.h"

/// <summary>
/// ImGui管理クラス
/// </summary>
namespace Engine {
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

	void CreateDescriptorHeap();

private:

	// SRV用デスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>srvHeap_;

	DirectXCommon* dxCommon_;

};

} // namespace Engine
