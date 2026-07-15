#pragma once
#include <memory>
#include <string>

#include "Sprite.h"
#include "TextTextureBaker.h"
#include "Vector2.h"
#include "Vector4.h"

namespace Engine {

/// <summary>
/// テキストを画像化してスプライトとして描画する高レベルクラス。
/// プログラムからは
///   TextSprite label;
///   TextRenderParams p; p.text = "SCORE"; p.pixelSize = 48; p.color = {1,1,0,1};
///   label.Initialize(p, {100, 40});
///   ...
///   label.SetText("SCORE 120");   // 文字を変える
///   label.SetColorMultiply({1,0,0,1}); // 色を変える（軽い・乗算）
///   label.SetPosition({200, 40}); // 位置を変える
///   label.Draw();
/// のように使える。文字/スタイルの変更は次の Draw までに自動で反映される。
/// </summary>
class TextSprite {
public:
    TextSprite() = default;

    /// <summary>テキストとスタイルからテクスチャを生成して初期化する。</summary>
    void Initialize(const TextRenderParams& params, Vector2 position = { 0.0f, 0.0f }, Vector2 anchor = { 0.0f, 0.0f });

    /// <summary>
    /// 保存済み PNG（resources/images/ 直下）を読み込んでテキストスプライトとして使う。
    /// エディタで保存した画像を、通常スプライトと同じ要領で呼び出すための入口。
    /// </summary>
    void InitializeFromFile(const std::string& imageName, Vector2 position = { 0.0f, 0.0f }, Vector2 anchor = { 0.0f, 0.0f });

    /// <summary>描画（必要なら再生成してから描く）。</summary>
    void Draw();

    // --- 文字・スタイル（変更すると次の Draw で再生成される） ---
    void SetText(const std::string& text);
    void SetParams(const TextRenderParams& params);
    void SetFillColor(const Vector4& rgba);   // 塗り色そのものを変える（再生成）
    void SetPixelSize(float px);
    void SetFont(const std::string& fontPath, const std::string& fallbackFontPath = "");
    void SetOutline(bool enabled, const Vector4& color, float width);
    const TextRenderParams& GetParams() const { return params_; }

    // --- 位置・色・見た目（軽い変更・再生成しない） ---
    void SetPosition(const Vector2& p);
    Vector2 GetPosition() const { return position_; }
    void SetColorMultiply(const Vector4& rgba); // スプライトの乗算色（塗りは維持したまま全体を着色）
    void SetScale(float s) { SetScale({ s, s }); }
    void SetScale(const Vector2& s);
    void SetRotation(float radian);
    void SetAnchor(const Vector2& a);

    // --- 参照 ---
    bool IsValid() const { return sprite_ != nullptr; }
    Sprite* GetSprite() { return sprite_.get(); }
    const std::string& GetTextureKey() const { return key_; }   // "resources/images/..."
    uint32_t GetSrvIndex() const;                                // ImGui プレビュー等で使用

private:
    void Rebake();       // params_ からテクスチャを作り直す
    void ApplyScale();   // テクスチャ実サイズ × scale_ を表示サイズへ反映

    TextRenderParams params_;
    std::unique_ptr<Sprite> sprite_;
    std::string key_;     // TextureManager 登録キー "resources/images/<name>"
    std::string name_;    // basePath 以下の名前 "<name>"
    Vector2 position_ = { 0.0f, 0.0f };
    Vector2 anchor_ = { 0.0f, 0.0f };
    Vector2 scale_ = { 1.0f, 1.0f };
    Vector4 multiply_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool dirty_ = false;   // 再生成が必要
    bool fromFile_ = false;

    static int s_counter_;
};

} // namespace Engine
