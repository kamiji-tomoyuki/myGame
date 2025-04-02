#pragma once
#include "d3d12.h"
#include "wrl.h"
#include "DirectXCommon.h"

#include "Vector4.h"

struct ConstBufferDataObjColor {
	Vector4 color_;
};

class ObjColor
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 行列の転送
	/// </summary>
	void TransferMatrix();

	/// 各ステータス取得関数
	/// <returns></returns>
	const Vector4& GetColor() const { return color_; }

	/// 各ステータス設定関数
	/// <returns></returns>
	void SetColor(const Vector4& color) { color_ = color; }

	/// <summary>
	/// グラフィックスコマンドの設定
	/// </summary>
	void SetGraphicCommand(UINT rootParameterIndex)const;

private:

	/// <summary>
	/// 定数バッファ生成
	/// </summary>
	void CreateConstBuffer();

	/// <summary>
	/// マッピング
	/// </summary>
	void Map();

private:

	DirectXCommon* dxCommon_ = nullptr;

	Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f, };

	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;

	ConstBufferDataObjColor* constMap_ = nullptr;
};

