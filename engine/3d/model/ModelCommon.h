#pragma once
#include "DirectXCommon.h"

// モデル管理
class ModelCommon
{
public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="dxCommon"></param>
	void Initialize();

public:

	/// 各ステータス取得関数
	/// <returns></returns>
	DirectXCommon* GetDxCommon()const { return dxCommon_; }

private:
	DirectXCommon* dxCommon_;
};

