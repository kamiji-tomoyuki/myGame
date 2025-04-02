#include"FullScreen.hlsli"

// テクスチャとサンプラー
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// ピクセルシェーダーの出力構造体
struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// Vignette効果のためのパラメータ
cbuffer VignetteParameter : register(b0)
{
    float vignetteStrength; // 明るさの強さ（中心部）
    float vignetteRadius; // フェードの範囲（周辺から中心に向けての影響範囲）
    float vignetteExponent; // 非線形調整（vignetteの強さを調整する）
    float2 vignetteCenter;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gTexture.Sample(gSampler, input.texcoord);
    
    // テクスチャ座標から計算してvignette効果を強調
    // テクスチャ座標が(0, 0)から(1, 1)に正規化されているので、それに基づいて周囲から中心に向けて減衰
    float2 center = vignetteCenter; // 画面の中心を基準に計算
    float2 offset = input.texcoord - center;

    // vignetteRadiusを使用して、フェードの影響範囲を決定
    float distanceFromCenter = length(offset) / vignetteRadius; // 中心からの距離
    distanceFromCenter = saturate(distanceFromCenter); // 0〜1の範囲に収める

    // vignetteStrengthを使用して、効果の強度を調整
    float vignette = (1.0f - distanceFromCenter) * vignetteStrength;

    // vignetteExponentを使用して、効果のカーブを調整
    vignette = saturate(pow(vignette, vignetteExponent)); // 非線形に調整

    // vignette効果をカラーに適用
    output.color.rgb *= vignette;

    return output;
}
