#pragma once
#include <string>

/// <summary>
/// 文字列型変換
/// </summary>
namespace StringUtility {
	// stringをwstringに変換する
	std::wstring ConvertString(const std::string& str);
	// wstringをstringに変換する
	std::string ConvertString(const std::wstring& str);
}
