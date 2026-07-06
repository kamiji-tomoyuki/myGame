#pragma once
#include <memory>
#include "DirectXCommon.h"
#include "PipeLineManager.h"

/// <summary>
/// スプライト共通部クラス
/// </summary>
namespace Engine {
class SpriteCommon
{
#pragma region シングルトンインスタンス
private:
	static std::unique_ptr<SpriteCommon> instance;

	SpriteCommon() = default;
	SpriteCommon(SpriteCommon&) = delete;
	SpriteCommon& operator = (SpriteCommon&) = delete;

public:
	~SpriteCommon() = default;
	// シングルトンインスタンスの取得
	static SpriteCommon* GetInstance();
	// 終了
	void Finalize();
#pragma endregion シングルトンインスタンス

public: // メンバ関数

	/// <summary>
	///  初期化
	/// </summary>
	void  Initialize();

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void DrawCommonSetting();

public:

	/// 各ステータス取得関数
	/// <returns></returns>
	DirectXCommon* GetDxCommon()const { return dxCommon_; }

	/// 各ステータス設定関数
	/// <returns></returns>
	void SetBlendMode(BlendMode blendMode);

private:
	DirectXCommon* dxCommon_;
	std::unique_ptr<PipeLineManager> psoManager_ = nullptr;

	// ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;

	// グラフィックスパイプライン
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState[5];
	BlendMode blendMode_ = BlendMode::kNormal;
};

} // namespace Engine
