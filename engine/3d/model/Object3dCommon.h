#pragma once
#include"DirectXCommon.h"
#include"PipeLineManager.h"
class Object3dCommon
{
#pragma region シングルトンインスタンス
private:
	static Object3dCommon* instance;

	Object3dCommon() = default;
	~Object3dCommon() = default;
	Object3dCommon(Object3dCommon&) = delete;
	Object3dCommon& operator = (Object3dCommon&) = delete;

public:
	// シングルトンインスタンスの取得
	static Object3dCommon* GetInstance();
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

	/// <summary>
	/// 共通描画設定(skinning)
	/// </summary>
	void skinningDrawCommonSetting();

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

	// objct3d
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState[5];

	// animation
	Microsoft::WRL::ComPtr<ID3D12RootSignature> skinningRootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> skinningGraphicsPipelineState = nullptr;

	BlendMode blendMode_ = BlendMode::kNormal;
};

