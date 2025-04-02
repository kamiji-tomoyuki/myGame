#pragma once
#include "Windows.h"
#include <cstdint>

#include "imgui.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// WindowsAPI
class WinApp
{
#pragma region シングルトンインスタンス
private:
	static WinApp* instance;

	WinApp() = default;
	~WinApp() = default;
	WinApp(WinApp&) = delete;
	WinApp& operator = (WinApp&) = delete;

public:
	// シングルトンインスタンスの取得
	static WinApp* GetInstance();
	// 終了
	void Finalize();
#pragma endregion シングルトンインスタンス

public: // 静的メンバ関数
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public: // メンバ関数

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// メッセージの処理
	/// </summary>
	/// <returns></returns>
	bool ProcessMessage();

public:

	/// 各ステータス取得関数
	/// <returns></returns>
	HWND GetHwnd() const { return hwnd; }
	HINSTANCE GetHInstance() const { return wc.hInstance; }

public: // 定数

	// クライアント領域のサイズ
	static const int32_t kClientWidth = 1280; // 横
	static const int32_t kClientHeight = 720; // 縦

private: // メンバ変数

	HWND hwnd = nullptr; // ウィンドウハンドル
	WNDCLASS wc{}; // ウィンドウクラスの設定
};

