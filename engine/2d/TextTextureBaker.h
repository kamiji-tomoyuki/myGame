#pragma once
#include <string>
#include <DirectXTex.h>

#include "Vector4.h"

namespace Engine {

/// <summary>
/// テキストを画像として焼き込むためのパラメータ。
/// フォント(.ttf)を指定し、文字サイズ・色・アウトラインなどを設定する。
/// </summary>
struct TextRenderParams {
    std::string text;                                  // 描画する文字列（UTF-8, 改行 '\n' 対応）
    std::string fontPath = "resources/fonts/PixelMplus12-Regular.ttf"; // 使用フォント(.ttf)
    std::string fallbackFontPath;                      // 主フォントに無い字を補うフォント（任意・空で無効）
    float   pixelSize = 64.0f;                          // 文字の高さ(px)
    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };         // 塗り色（RGBA 0-1）
    bool    outlineEnabled = false;                     // アウトラインを付けるか
    Vector4 outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };  // アウトライン色（RGBA 0-1）
    float   outlineWidth = 3.0f;                        // アウトラインの太さ(px)
    float   lineSpacing = 1.0f;                         // 行間倍率（1.0=フォント標準）
    int     alignment = 0;                              // 複数行の揃え 0=左 1=中央 2=右
    int     padding = 6;                                // 余白(px)。アウトライン分は自動加算される
};

/// <summary>
/// TTF フォントから文字列を RGBA テクスチャ画像へラスタライズするユーティリティ。
/// stb_truetype を利用し、DirectXTex の ScratchImage / PNG として出力できる。
/// </summary>
class TextTextureBaker {
public:
    /// <summary>
    /// params.text を画像へ焼き込む。
    /// 出力フォーマットは DXGI_FORMAT_R8G8B8A8_UNORM_SRGB（ストレートアルファ）。
    /// </summary>
    /// <returns>成功したら true。フォント読込失敗や空文字なら false</returns>
    static bool Bake(const TextRenderParams& params, DirectX::ScratchImage& out);

    /// <summary>
    /// params.text を焼き込んで PNG ファイルとして保存する（親フォルダは自動生成）。
    /// </summary>
    static bool BakeToPng(const TextRenderParams& params, const std::string& filePath);
};

} // namespace Engine
