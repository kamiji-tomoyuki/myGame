#pragma once
#include "WinApp.h"

/// <summary>
/// UI レイアウト用の共通定数
/// 画面サイズは WinApp::kClientWidth / kClientHeight を唯一の基準とし、
/// 画面中央・右端・下端などはそこから導出する（解像度変更時に追従させるため）。
/// </summary>
namespace UILayout {

	// --- 画面サイズ ---
	inline constexpr float kScreenWidth = static_cast<float>(Engine::WinApp::kClientWidth);
	inline constexpr float kScreenHeight = static_cast<float>(Engine::WinApp::kClientHeight);
	inline constexpr float kScreenCenterX = kScreenWidth * 0.5f;
	inline constexpr float kScreenCenterY = kScreenHeight * 0.5f;

	// --- 画面端からの共通マージン ---
	inline constexpr float kScreenMargin = 40.0f;

	// --- HPバー（プレイヤー＝左寄せ / 敵＝右寄せ の左右対称配置） ---
	inline constexpr float kHpBarWidth = 350.0f;
	inline constexpr float kHpBarHeight = 40.0f;
	inline constexpr float kHpBarBgPadding = 4.0f;
	inline constexpr float kHpBarTopY = 150.0f;
	inline constexpr float kHpBarPlayerX = kScreenMargin;                // 左端基準（アンカー 0,0）
	inline constexpr float kHpBarEnemyX = kScreenWidth - kScreenMargin;  // 右端基準（アンカー 1,0）

} // namespace UILayout
