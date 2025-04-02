#pragma once
#include "PipeLineManager.h"
#include "DirectXCommon.h"

// パーティクル
class ParticleCommon
{
#pragma region シングルトンインスタンス
private:
	static ParticleCommon* instance;

	ParticleCommon() = default;
	~ParticleCommon() = default;
	ParticleCommon(ParticleCommon&) = delete;
	ParticleCommon& operator = (ParticleCommon&) = delete;

public:
	// シングルトンインスタンスの取得
	static ParticleCommon* GetInstance();
	// 終了
	void Finalize();
#pragma endregion シングルトンインスタンス

public:
	
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(DirectXCommon* dxCommon);

	/// <summary>
	/// 共通描画処理
	/// </summary>
	void DrawCommonSetting();

	/// 各ステータス取得関数
	/// <returns></returns>
	DirectXCommon* GetDxCommon()const { return dxCommon_; }

	/// 各ステータス設定関数
	/// <returns></returns>
	void SetBlendMode(BlendMode blendMode);

private:

	DirectXCommon* dxCommon_;
	std::unique_ptr<PipeLineManager>psoManager_;
	
	// ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	// グラフィックスパイプライン
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState[5];

	BlendMode blendMode_ = BlendMode::kAdd;
};

