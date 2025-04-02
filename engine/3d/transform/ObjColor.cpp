#include "ObjColor.h"

void ObjColor::Initialize()
{

	dxCommon_ = DirectXCommon::GetInstance();

	color_ = { 1.0f,1.0f,1.0f,1.0f };
	CreateConstBuffer();
	Map();
	TransferMatrix();
}

void ObjColor::TransferMatrix()
{
	if (constMap_) {
		constMap_->color_ = color_;
	}
}

void ObjColor::SetGraphicCommand(UINT rootParameterIndex) const
{
	dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(rootParameterIndex, constBuffer_->GetGPUVirtualAddress());
}

void ObjColor::CreateConstBuffer()
{
	const UINT bufferSize = sizeof(ConstBufferDataObjColor);
	constBuffer_ = dxCommon_->CreateBufferResource(bufferSize);
}

void ObjColor::Map()
{
	HRESULT hr = constBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&constMap_));
	if (FAILED(hr)) {
		throw std::runtime_error("Failed to map constant buffer.");
	}
}
