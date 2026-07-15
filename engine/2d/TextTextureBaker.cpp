// windows.h の min/max マクロが std::max/std::min と衝突するため無効化（最初の include より前）。
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "TextTextureBaker.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>

#include "StringUtility.h"

// stb_truetype の実装を本 TU に取り込む。
// ImGui 側は STBTT_STATIC 付きで別 TU に取り込んでおり（imgui_draw.cpp）、
// こちらも STBTT_STATIC で内部リンケージにするためシンボル衝突は起きない。
// サードパーティ実装なので、本プロジェクトの /WX(警告=エラー) を避けるため
// 取り込み範囲だけ警告を抑制する。
#pragma warning(push, 0)
#pragma warning(disable: 4244 4245 4267 4456 4457 4701 4703 4996 4146 4127)
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "imstb_truetype.h"
#pragma warning(pop)

namespace Engine {

namespace {

// フォントファイルのバイト列をパスごとにキャッシュ（プレビュー再生成の連続読込を軽くする）。
std::unordered_map<std::string, std::vector<unsigned char>>& FontCache() {
    static std::unordered_map<std::string, std::vector<unsigned char>> cache;
    return cache;
}

const std::vector<unsigned char>* LoadFontBytes(const std::string& path) {
    if (path.empty()) { return nullptr; }
    auto& cache = FontCache();
    auto it = cache.find(path);
    if (it != cache.end()) {
        return it->second.empty() ? nullptr : &it->second;
    }
    std::vector<unsigned char> bytes;
    std::ifstream ifs(std::filesystem::path(StringUtility::ConvertString(path)), std::ios::binary);
    if (ifs) {
        ifs.seekg(0, std::ios::end);
        std::streamoff size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        if (size > 0) {
            bytes.resize(static_cast<size_t>(size));
            ifs.read(reinterpret_cast<char*>(bytes.data()), size);
        }
    }
    auto& stored = cache[path] = std::move(bytes);
    return stored.empty() ? nullptr : &stored;
}

// UTF-8 文字列をコードポイント列へデコード（不正バイトは U+FFFD 扱い）。
std::vector<uint32_t> DecodeUtf8(const std::string& s) {
    std::vector<uint32_t> out;
    out.reserve(s.size());
    size_t i = 0;
    const size_t n = s.size();
    while (i < n) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        uint32_t cp = 0;
        int extra = 0;
        if (c < 0x80) { cp = c; extra = 0; }
        else if ((c >> 5) == 0x6) { cp = c & 0x1F; extra = 1; }
        else if ((c >> 4) == 0xE) { cp = c & 0x0F; extra = 2; }
        else if ((c >> 3) == 0x1E) { cp = c & 0x07; extra = 3; }
        else { cp = 0xFFFD; extra = 0; }
        ++i;
        for (int k = 0; k < extra; ++k) {
            if (i >= n || (static_cast<unsigned char>(s[i]) & 0xC0) != 0x80) { cp = 0xFFFD; break; }
            cp = (cp << 6) | (static_cast<unsigned char>(s[i]) & 0x3F);
            ++i;
        }
        out.push_back(cp);
    }
    return out;
}

// 1 文字分のフォント・グリフ情報。
struct GlyphRef {
    const stbtt_fontinfo* font = nullptr;
    float scale = 0.0f;
    int   glyph = 0;
};

inline float Saturate(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
inline uint8_t ToByte(float v01) { return static_cast<uint8_t>(Saturate(v01) * 255.0f + 0.5f); }

} // namespace

bool TextTextureBaker::Bake(const TextRenderParams& params, DirectX::ScratchImage& out) {
    if (params.text.empty()) { return false; }

    const std::vector<unsigned char>* primaryBytes = LoadFontBytes(params.fontPath);
    if (!primaryBytes) { return false; }

    stbtt_fontinfo primary{};
    if (!stbtt_InitFont(&primary, primaryBytes->data(),
                        stbtt_GetFontOffsetForIndex(primaryBytes->data(), 0))) {
        return false;
    }

    // フォールバックフォント（主フォントに字が無いときに使う）
    const std::vector<unsigned char>* fbBytes =
        (params.fallbackFontPath != params.fontPath) ? LoadFontBytes(params.fallbackFontPath) : nullptr;
    stbtt_fontinfo fallback{};
    bool hasFallback = false;
    if (fbBytes) {
        hasFallback = stbtt_InitFont(&fallback, fbBytes->data(),
                                     stbtt_GetFontOffsetForIndex(fbBytes->data(), 0)) != 0;
    }

    const float pixelSize = std::max(4.0f, params.pixelSize);
    const float primaryScale = stbtt_ScaleForPixelHeight(&primary, pixelSize);
    const float fallbackScale = hasFallback ? stbtt_ScaleForPixelHeight(&fallback, pixelSize) : 0.0f;

    int ascentI = 0, descentI = 0, lineGapI = 0;
    stbtt_GetFontVMetrics(&primary, &ascentI, &descentI, &lineGapI);
    const float ascent = ascentI * primaryScale;
    const float descent = descentI * primaryScale; // 負値
    const float lineGap = lineGapI * primaryScale;
    const float lineAdvance = (ascent - descent + lineGap) * std::max(0.1f, params.lineSpacing);

    // 1 文字を解決（主フォント優先、無ければフォールバック）
    auto resolveGlyph = [&](uint32_t cp) -> GlyphRef {
        int g = stbtt_FindGlyphIndex(&primary, static_cast<int>(cp));
        if (g != 0 || !hasFallback) { return { &primary, primaryScale, g }; }
        int gf = stbtt_FindGlyphIndex(&fallback, static_cast<int>(cp));
        if (gf != 0) { return { &fallback, fallbackScale, gf }; }
        return { &primary, primaryScale, g };
    };

    // 行へ分割
    std::vector<std::vector<uint32_t>> lines;
    lines.emplace_back();
    for (uint32_t cp : DecodeUtf8(params.text)) {
        if (cp == '\r') { continue; }
        if (cp == '\n') { lines.emplace_back(); continue; }
        lines.back().push_back(cp);
    }

    // 各行の幅を計測
    auto measureLine = [&](const std::vector<uint32_t>& line) -> float {
        float w = 0.0f;
        for (size_t i = 0; i < line.size(); ++i) {
            GlyphRef gr = resolveGlyph(line[i]);
            int adv = 0, lsb = 0;
            stbtt_GetGlyphHMetrics(gr.font, gr.glyph, &adv, &lsb);
            w += adv * gr.scale;
            if (i + 1 < line.size()) {
                GlyphRef nx = resolveGlyph(line[i + 1]);
                if (nx.font == gr.font) {
                    w += stbtt_GetGlyphKernAdvance(gr.font, gr.glyph, nx.glyph) * gr.scale;
                }
            }
        }
        return w;
    };

    float maxLineWidth = 0.0f;
    std::vector<float> lineWidths(lines.size(), 0.0f);
    for (size_t l = 0; l < lines.size(); ++l) {
        lineWidths[l] = measureLine(lines[l]);
        maxLineWidth = std::max(maxLineWidth, lineWidths[l]);
    }

    // 余白（アウトラインの太さ分も確保）
    const int outlineR = params.outlineEnabled ? static_cast<int>(std::ceil(std::max(0.0f, params.outlineWidth))) : 0;
    const int margin = std::max(0, params.padding) + outlineR + 2;

    const int textHeight = static_cast<int>(std::ceil((ascent - descent) + lineAdvance * static_cast<float>(lines.size() - 1)));
    const int W = std::max(1, static_cast<int>(std::ceil(maxLineWidth)) + margin * 2);
    const int H = std::max(1, textHeight + margin * 2);

    // グリフのカバレッジ（アルファ）を貯めるバッファ
    std::vector<uint8_t> cov(static_cast<size_t>(W) * H, 0);

    auto blitGlyph = [&](const uint8_t* src, int gw, int gh, int dstX, int dstY) {
        for (int y = 0; y < gh; ++y) {
            int py = dstY + y;
            if (py < 0 || py >= H) { continue; }
            for (int x = 0; x < gw; ++x) {
                int px = dstX + x;
                if (px < 0 || px >= W) { continue; }
                uint8_t v = src[y * gw + x];
                uint8_t& d = cov[static_cast<size_t>(py) * W + px];
                if (v > d) { d = v; }
            }
        }
    };

    std::vector<uint8_t> glyphBuf;
    for (size_t l = 0; l < lines.size(); ++l) {
        const auto& line = lines[l];
        float alignOffset = 0.0f;
        if (params.alignment == 1) { alignOffset = (maxLineWidth - lineWidths[l]) * 0.5f; }
        else if (params.alignment == 2) { alignOffset = (maxLineWidth - lineWidths[l]); }

        float penX = margin + alignOffset;
        const float baselineY = margin + ascent + lineAdvance * static_cast<float>(l);

        for (size_t i = 0; i < line.size(); ++i) {
            GlyphRef gr = resolveGlyph(line[i]);
            int adv = 0, lsb = 0;
            stbtt_GetGlyphHMetrics(gr.font, gr.glyph, &adv, &lsb);

            int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
            stbtt_GetGlyphBitmapBox(gr.font, gr.glyph, gr.scale, gr.scale, &x0, &y0, &x1, &y1);
            const int gw = x1 - x0;
            const int gh = y1 - y0;
            if (gw > 0 && gh > 0) {
                glyphBuf.assign(static_cast<size_t>(gw) * gh, 0);
                stbtt_MakeGlyphBitmap(gr.font, glyphBuf.data(), gw, gh, gw, gr.scale, gr.scale, gr.glyph);
                const int dstX = static_cast<int>(std::floor(penX)) + x0;
                const int dstY = static_cast<int>(std::floor(baselineY)) + y0;
                blitGlyph(glyphBuf.data(), gw, gh, dstX, dstY);
            }

            penX += adv * gr.scale;
            if (i + 1 < line.size()) {
                GlyphRef nx = resolveGlyph(line[i + 1]);
                if (nx.font == gr.font) {
                    penX += stbtt_GetGlyphKernAdvance(gr.font, gr.glyph, nx.glyph) * gr.scale;
                }
            }
        }
    }

    // アウトライン用のカバレッジ（cov を円形に膨張）
    std::vector<uint8_t> ocov;
    if (params.outlineEnabled && outlineR > 0) {
        // 円形の構造要素オフセットを先に用意
        std::vector<std::pair<int, int>> offsets;
        const float rEdge = params.outlineWidth + 0.5f;
        for (int dy = -outlineR; dy <= outlineR; ++dy) {
            for (int dx = -outlineR; dx <= outlineR; ++dx) {
                if (std::sqrt(static_cast<float>(dx * dx + dy * dy)) <= rEdge) {
                    offsets.emplace_back(dx, dy);
                }
            }
        }
        ocov.assign(static_cast<size_t>(W) * H, 0);
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                uint8_t m = 0;
                for (const auto& o : offsets) {
                    int sx = x + o.first;
                    int sy = y + o.second;
                    if (sx < 0 || sx >= W || sy < 0 || sy >= H) { continue; }
                    uint8_t v = cov[static_cast<size_t>(sy) * W + sx];
                    if (v > m) { m = v; }
                }
                ocov[static_cast<size_t>(y) * W + x] = m;
            }
        }
    }

    // RGBA へ合成
    if (FAILED(out.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, W, H, 1, 1))) {
        return false;
    }
    const DirectX::Image* img = out.GetImage(0, 0, 0);
    uint8_t* dst = img->pixels;
    const size_t rowPitch = img->rowPitch;

    for (int y = 0; y < H; ++y) {
        uint8_t* row = dst + static_cast<size_t>(y) * rowPitch;
        for (int x = 0; x < W; ++x) {
            const size_t idx = static_cast<size_t>(y) * W + x;
            const float fillA = (cov[idx] / 255.0f) * params.color.w;

            float r, g, b, a;
            if (params.outlineEnabled && !ocov.empty()) {
                const float outA = (ocov[idx] / 255.0f) * params.outlineColor.w;
                // アウトラインを下地、その上に塗りを over 合成
                float curR = params.outlineColor.x, curG = params.outlineColor.y, curB = params.outlineColor.z;
                float curA = outA;
                const float na = fillA + curA * (1.0f - fillA);
                if (na > 0.0001f) {
                    r = (params.color.x * fillA + curR * curA * (1.0f - fillA)) / na;
                    g = (params.color.y * fillA + curG * curA * (1.0f - fillA)) / na;
                    b = (params.color.z * fillA + curB * curA * (1.0f - fillA)) / na;
                } else {
                    r = g = b = 0.0f;
                }
                a = na;
            } else {
                r = params.color.x; g = params.color.y; b = params.color.z; a = fillA;
            }

            uint8_t* p = row + static_cast<size_t>(x) * 4;
            p[0] = ToByte(r);
            p[1] = ToByte(g);
            p[2] = ToByte(b);
            p[3] = ToByte(a);
        }
    }

    return true;
}

bool TextTextureBaker::BakeToPng(const TextRenderParams& params, const std::string& filePath) {
    DirectX::ScratchImage image;
    if (!Bake(params, image)) { return false; }

    try {
        std::filesystem::path p(StringUtility::ConvertString(filePath));
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
    } catch (...) {}

    const std::wstring wpath = StringUtility::ConvertString(filePath);
    HRESULT hr = DirectX::SaveToWICFile(
        *image.GetImage(0, 0, 0),
        DirectX::WIC_FLAGS_NONE,
        DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
        wpath.c_str());
    return SUCCEEDED(hr);
}

} // namespace Engine
