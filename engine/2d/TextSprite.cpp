#include "TextSprite.h"

#include <DirectXTex.h>

#include "TextureManager.h"

namespace Engine {

int TextSprite::s_counter_ = 0;

void TextSprite::Initialize(const TextRenderParams& params, Vector2 position, Vector2 anchor) {
    params_ = params;
    position_ = position;
    anchor_ = anchor;
    fromFile_ = false;

    // このインスタンス専用の登録キーを割り当てる（メモリ生成テクスチャ用）。
    name_ = "__textsprite_" + std::to_string(s_counter_++) + ".png";
    key_ = "resources/images/" + name_;

    Rebake();
}

void TextSprite::InitializeFromFile(const std::string& imageName, Vector2 position, Vector2 anchor) {
    position_ = position;
    anchor_ = anchor;
    fromFile_ = true;
    name_ = imageName;
    key_ = "resources/images/" + imageName;

    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize(imageName, position_, multiply_, anchor_);
    ApplyScale();
    dirty_ = false;
}

void TextSprite::Rebake() {
    if (fromFile_) { return; }

    DirectX::ScratchImage image;
    if (!TextTextureBaker::Bake(params_, image)) {
        // 生成失敗（空文字・フォント読込失敗など）。既存表示は維持する。
        dirty_ = false;
        return;
    }

    TextureManager::GetInstance()->RegisterTextureFromImage(key_, image);

    if (!sprite_) {
        sprite_ = std::make_unique<Sprite>();
        sprite_->InitializeWithRegisteredTexture(name_, position_, multiply_, anchor_);
    } else {
        sprite_->FitToTexture();   // サイズが変わっている場合に合わせ直す
    }
    ApplyScale();
    dirty_ = false;
}

void TextSprite::ApplyScale() {
    if (!sprite_) { return; }
    const Vector2 tex = sprite_->GetTexSize();
    sprite_->SetSize({ tex.x * scale_.x, tex.y * scale_.y });
}

void TextSprite::Draw() {
    if (dirty_) { Rebake(); }
    if (sprite_) { sprite_->Draw(); }
}

// ---- 文字・スタイル（再生成） ----
void TextSprite::SetText(const std::string& text) {
    if (params_.text == text) { return; }
    params_.text = text;
    dirty_ = true;
}

void TextSprite::SetParams(const TextRenderParams& params) {
    params_ = params;
    dirty_ = true;
}

void TextSprite::SetFillColor(const Vector4& rgba) {
    params_.color = rgba;
    dirty_ = true;
}

void TextSprite::SetPixelSize(float px) {
    params_.pixelSize = px;
    dirty_ = true;
}

void TextSprite::SetFont(const std::string& fontPath, const std::string& fallbackFontPath) {
    params_.fontPath = fontPath;
    params_.fallbackFontPath = fallbackFontPath;
    dirty_ = true;
}

void TextSprite::SetOutline(bool enabled, const Vector4& color, float width) {
    params_.outlineEnabled = enabled;
    params_.outlineColor = color;
    params_.outlineWidth = width;
    dirty_ = true;
}

// ---- 位置・色・見た目（軽い変更） ----
void TextSprite::SetPosition(const Vector2& p) {
    position_ = p;
    if (sprite_) { sprite_->SetPosition(p); }
}

void TextSprite::SetColorMultiply(const Vector4& rgba) {
    multiply_ = rgba;
    if (sprite_) {
        sprite_->SetColor({ rgba.x, rgba.y, rgba.z });
        sprite_->SetAlpha(rgba.w);
    }
}

void TextSprite::SetScale(const Vector2& s) {
    scale_ = s;
    ApplyScale();
}

void TextSprite::SetRotation(float radian) {
    if (sprite_) { sprite_->SetRotation(radian); }
}

void TextSprite::SetAnchor(const Vector2& a) {
    anchor_ = a;
    if (sprite_) { sprite_->SetAnchorPoint(a); }
}

uint32_t TextSprite::GetSrvIndex() const {
    return TextureManager::GetInstance()->GetTextureIndexByFilePath(key_);
}

} // namespace Engine
