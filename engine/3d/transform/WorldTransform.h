#pragma once
#include "d3d12.h"
#include "wrl.h"
#include "DirectXCommon.h"

#include "myMath.h"

struct ConstBufferDataWorldTransform {
	Matrix4x4 matWorld;
};

class WorldTransform
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

	/// <summary>
	/// 定数バッファ生成
	/// </summary>
	void CreateConstBuffer();

	/// <summary>
	/// マッピング
	/// </summary>
	void Map();

	/// <summary>
	/// 行列の計算・転送
	/// </summary>
	void UpdateMatrix();

	/// <summary>
	/// 定数バッファの取得
	/// </summary>
	/// <returns></returns>
	const Microsoft::WRL::ComPtr<ID3D12Resource>& GetConstBuffer() const { return constBuffer_; }

public:

	// --- ローカル座標情報 ---
	Vector3 scale_ = { 1.0f,1.0f,1.0f };
	Vector3 rotation_ = { 0.0f,0.0f,0.0f };
	Vector3 translation_ = { 0.0f,0.0f,0.0f };

	Matrix4x4 matWorld_;

	const WorldTransform* parent_ = nullptr;

	WorldTransform() = default;
	~WorldTransform() = default;

private:

	DirectXCommon* dxCommon_ = nullptr;

	// 定数バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource>constBuffer_;
	// マッピング済み
	ConstBufferDataWorldTransform* constMap = nullptr;
	//// コピー禁止
	//WorldTransform(const WorldTransform&) = delete;
	//WorldTransform& operator=(const WorldTransform&) = delete;
};
//static_assert(!std::is_copy_assignable_v<WorldTransform>);
