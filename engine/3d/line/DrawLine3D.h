#pragma once
#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <string>
#include <wrl/client.h>

#include "ViewProjection.h"
#include "PipeLineManager.h"

#include <Vector3.h>
#include <Vector4.h>
#include <Matrix4x4.h>

using namespace Microsoft::WRL;

// ライン描画
class DrawLine3D 
{
#pragma region シングルトンインスタンス
private:
	static DrawLine3D* instance;

	DrawLine3D() = default;
	~DrawLine3D() = default;
	DrawLine3D(DrawLine3D&) = delete;
	DrawLine3D& operator = (DrawLine3D&) = delete;

public:
	// シングルトンインスタンスの取得
	static DrawLine3D* GetInstance();
	// 終了
	void Finalize();
#pragma endregion シングルトンインスタンス

public: // メンバ関数
	
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// ポイント指定
	/// </summary>
	void SetPoints(const Vector3& p1, const Vector3& p2, const Vector4& color = { 1.0f,1.0f,1.0f,1.0f });

	/// <summary>
	/// リセット
	/// </summary>
	void Reset();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw(const ViewProjection& viewProjection);

public: // メンバ変数

	static const UINT kMaxLineCount = 5000;
	static const UINT kVertexCountLine = 2;
	static const UINT kIndexCountLine = 0;

	struct VertexPosColor {
		Vector3 pos;
		Vector4 color;
	};

	struct LineData {
		ComPtr<ID3D12Resource> vertBuffer;
		ComPtr<ID3D12Resource> indexBuffer;
		D3D12_VERTEX_BUFFER_VIEW vbView{};
		D3D12_INDEX_BUFFER_VIEW ibView{};
		VertexPosColor* vertMap = nullptr;
		uint16_t* indexMap = nullptr;
	};

	std::unique_ptr<LineData> CreateMesh(UINT vertexCount, UINT indexCount);

private:

	/// <summary>
	/// メッシュ生成
	/// </summary>
	void CreateMeshes();

	/// <summary>
	/// リソースデータ作成
	/// </summary>
	void CreateResource();

private:

	std::unique_ptr<LineData> line_;
	uint32_t indexLine_ = 0;

	std::unique_ptr<PipeLineManager> psoManager_ = nullptr;

	DirectXCommon* dxCommon;

	// ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;

	// グラフィックスパイプライン
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;

	struct CBuffer {
		Matrix4x4 viewProject;
	};
	CBuffer* cBufferData_ = nullptr;
	ComPtr<ID3D12Resource> cBufferResource_ = nullptr;
};