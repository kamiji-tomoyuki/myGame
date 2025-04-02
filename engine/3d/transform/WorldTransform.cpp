#include "WorldTransform.h"

void WorldTransform::Initialize()
{
	scale_ = { 1.0f, 1.0f, 1.0f };
	rotation_ = { 0.0f, 0.0f, 0.0f };
	translation_ = { 0.0f, 0.0f, 0.0f };
	dxCommon_ = DirectXCommon::GetInstance();
	matWorld_ = MakeIdentity4x4();

	CreateConstBuffer();
	Map();
	UpdateMatrix();
}

void WorldTransform::TransferMatrix()
{
	if (constMap) {
		constMap->matWorld = matWorld_;
	}
}

void WorldTransform::CreateConstBuffer()
{
	const UINT bufferSize = sizeof(ConstBufferDataWorldTransform);
	constBuffer_ = dxCommon_->CreateBufferResource(bufferSize);
}

void WorldTransform::Map()
{
	HRESULT hr = constBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&constMap));
}

void WorldTransform::UpdateMatrix()
{
	matWorld_ = MakeAffineMatrix(scale_, rotation_, translation_);

	if (parent_) {
		matWorld_ *= parent_->matWorld_;
	}

	TransferMatrix();
}

