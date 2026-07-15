#pragma once
#include <memory>
#include "d3d12.h"
#include "DirectXCommon.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "SrvManager.h"
#include "string"
#include "unordered_map"
#include "wrl.h"

/// <summary>
/// テクスチャ管理クラス
/// </summary>
namespace Engine {
class TextureManager
{
public:
	~TextureManager() = default;

private:
	static std::unique_ptr<TextureManager> instance;

	TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(TextureManager&) = delete;


public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(SrvManager* srvManager);

	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns></returns>
	static TextureManager* GetInstance();

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

	/// <summary>
	/// テクスチャファイルの読み込み
	/// </summary>
	/// <param name="filePath"></param>
	/// <returns></returns>
	void LoadTexture(const std::string& filePath);

	/// <summary>
	/// SRVインデックスの開始番号
	/// </summary>
	/// <param name="filePath"></param>
	/// <returns></returns>
	uint32_t GetTextureIndexByFilePath(const std::string& filePath);

	/// <summary>
	/// モデル用テクスチャファイルの読み込み
	/// </summary>
	/// <param name="filePath"></param>
	/// <returns></returns>
	void LoadModelTexture(const std::string& filePath);

	/// <summary>
	/// モデル用SRVインデックスの開始番号
	/// </summary>
	/// <param name="filePath"></param>
	/// <returns></returns>
	uint32_t GetModelTextureIndexByFilePath(const std::string& filePath);

	/// <summary>
	/// メモリ上の画像（DirectX::ScratchImage）からテクスチャを登録／更新する。
	/// テキスト画像などをその場で生成してスプライトに使うための入口。
	/// - 未登録のキーなら新規に SRV を確保して登録する。
	/// - 既存キーなら同じ SRV スロットを再利用し、リソースだけ差し替える
	///   （プレビューの再生成で毎回 SRV を消費しないため）。
	/// キーは "resources/images/&lt;名前&gt;" 形式で渡す（Sprite の参照キーと一致させる）。
	/// ※ アップロードはメインのコマンドリストに積まれるため、フレームの更新中に呼ぶこと。
	///   同一キーへ1フレーム内で複数回呼ばないこと（中間リソースが破棄され得るため）。
	/// </summary>
	void RegisterTextureFromImage(const std::string& fullPathKey, const DirectX::ScratchImage& image);

	/// <summary>指定キーのテクスチャが登録済みか</summary>
	bool HasTexture(const std::string& fullPathKey) const { return textureDatas.contains(fullPathKey); }

	/// <summary>
	/// テクスチャ番号からGPUハンドルを取得
	/// </summary>
	/// <param name="textureIndex"></param>
	/// <returns></returns>
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath);

	/// <summary>
	/// メタデータを取得
	/// </summary>
	/// <param name="textureIndex"></param>
	/// <returns></returns>
	const DirectX::TexMetadata& GetMetaData(const std::string& filePath);

	SrvManager* GetSrvManager() { return srvManager_; }

private:
	// テクスチャ１枚分のデータ
	struct TextureData {
		DirectX::TexMetadata metadata;                   // 画像の幅や高さなどの情報
		Microsoft::WRL::ComPtr<ID3D12Resource> resource; // テクスチャリソース
		uint32_t srvIndex;
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;        // SRV作成時に必要なCPUハンドル
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;        // 描画コマンドに必要なGPUハンドル
	};
	std::unordered_map<std::string, TextureData> textureDatas;	         // テクスチャデータ

	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	// SRVインデックスの開始番号（ImGuiで0番を使用するため、1番から使用）
	static constexpr uint32_t kSRVIndexTop = 1;
};

} // namespace Engine
