#include "Sprite.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

#include "myMath.h"

void Sprite::Initialize(const std::string& textureFilePath, Vector2 position, Vector4 color, Vector2 anchorpoint, bool isFlipX, bool isFlipY)
{
	// --- 引数で受け取りメンバ変数に記録 ---
	this->spriteCommon_ = SpriteCommon::GetInstance();

	// --- パスを設定 ---
	fullpath = basePath_ + textureFilePath;

	// --- 各データ生成 ---
	CreateVartexData();
	CreateMaterialData();
	CreateTransformationMatrixData();

	// --- テクスチャ読み込み ---
	TextureManager::GetInstance()->LoadTexture(fullpath);

	// --- その他引数の適応 ---
	position_ = position;
	materialData->color = color;
	anchorPoint_ = anchorpoint;
	isFlipX_ = isFlipX;
	isFlipY_ = isFlipY;

	// --- 切り取り ---
	AdjustTextureSize();
}

void Sprite::Update()
{
	// --- アンカーポイントを考慮した頂点座標の計算 ---
	// サイズを考慮してアンカーポイント分オフセット
	left = (0.0f - anchorPoint_.x) * size.x;
	right = (1.0f - anchorPoint_.x) * size.x;
	top = (0.0f - anchorPoint_.y) * size.y;
	bottom = (1.0f - anchorPoint_.y) * size.y;

	// --- フリップの更新処理 ---
	if (isFlipX_) {
		std::swap(left, right);
	}
	if (isFlipY_) {
		std::swap(top, bottom);
	}

	// --- world座標変換 ---
	Transform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	transform.translate = { position_.x, position_.y, 0.0f };
	transform.rotate = { 0.0f, 0.0f, rotation };
	transform.scale = { 1.0f, 1.0f, 1.0f }; // サイズは頂点座標に含まれるので1.0f

	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 viewMatrix = MakeIdentity4x4();
	Matrix4x4 projectionMatrix = MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
	Matrix4x4 worldProjectionMatrix = (worldMatrix * (viewMatrix * projectionMatrix));

	// --- transformationMatrixDataの更新 ---
	transformationMatrixData->WVP = worldProjectionMatrix;
	transformationMatrixData->World = worldMatrix;

	// --- テクスチャ範囲指定の更新処理 ---
	const DirectX::TexMetadata& metadata =
		TextureManager::GetInstance()->GetMetaData(fullpath);
	float tex_left = textureLeftTop.x / metadata.width;
	float tex_right = (textureLeftTop.x + textureSize.x) / metadata.width;
	float tex_top = textureLeftTop.y / metadata.height;
	float tex_bottom = (textureLeftTop.y + textureSize.y) / metadata.height;

	// --- vertexDataに割り当てる ---
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	vertexData[0].position = { left, bottom, 0.0f, 1.0f }; // 左下
	vertexData[0].texcoord = { tex_left, tex_bottom };

	vertexData[1].position = { left, top, 0.0f, 1.0f }; // 左上
	vertexData[1].texcoord = { tex_left, tex_top };

	vertexData[2].position = { right, bottom, 0.0f, 1.0f }; // 右下
	vertexData[2].texcoord = { tex_right, tex_bottom };

	vertexData[3].position = { right, top, 0.0f, 1.0f }; // 右上
	vertexData[3].texcoord = { tex_right, tex_top };

	// --- indexDataに割り当てる ---
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	indexData[0] = 0; indexData[1] = 1; indexData[2] = 2;
	indexData[3] = 1; indexData[4] = 3; indexData[5] = 2;
}

void Sprite::Draw()
{
	// --- 更新処理 ---
	Update();

	// --- vertexBufferViewの生成 ---
	spriteCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定
	// --- indexBufferViewの生成 ---
	spriteCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&indexBufferView);

	// --- マテリアルCBufferの場所を設定 ---
	spriteCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

	// --- 座標変換行列CBufferの場所を設定 ---
	spriteCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResource->GetGPUVirtualAddress());

	// --- SRVのDescriptorTableを設定 ---
	srvManager_ = TextureManager::GetInstance()->GetSrvManager();
	srvManager_->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetTextureIndexByFilePath(fullpath));

	// --- 描画(DrawCall/ドローコール) ---
	spriteCommon_->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Sprite::SetTexturePath(std::string textureFilePath)
{
	fullpath = basePath_ + textureFilePath;
	TextureManager::GetInstance()->GetTextureIndexByFilePath(fullpath);
}

void Sprite::CreateVartexData()
{
	// --- vertexResourceの作成 ---
	vertexResource = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * 6);
	// --- vertexBufferViewの作成 ---
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 6;
	vertexBufferView.StrideInBytes = sizeof(VertexData);
	// --- vertexDataの設定 ---
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	// --- indexResourceの作成 ---
	indexResource = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * 6);
	// --- indexBufferViewの作成 ---
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	// --- indexDataの設定 ---
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
}

void Sprite::CreateMaterialData()
{
	// --- materialResourceの作成 ---
	materialResource = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));
	// --- materialDataの設定 ---
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->uvTransform = MakeIdentity4x4();
}

void Sprite::CreateTransformationMatrixData()
{
	// --- transformationMatrixResourceの作成 ---
	transformationMatrixResource = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
	// --- transformationMatrixDataに割り当てる ---
	transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
	transformationMatrixData->WVP = MakeIdentity4x4();
	transformationMatrixData->World = MakeIdentity4x4();
}

void Sprite::AdjustTextureSize()
{
	// テクスチャメタデータを取得
	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(fullpath);

	textureSize.x = static_cast<float>(metadata.width);
	textureSize.y = static_cast<float>(metadata.height);
	// 画像サイズをテクスチャサイズに合わせる
	size = textureSize;
}