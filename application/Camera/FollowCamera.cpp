#include "FollowCamera.h"
#include <Xinput.h>
#include <Input.h>

#include "myMath.h"

void FollowCamera::Initialize()
{
	vp_.Initialize();
}

void FollowCamera::Update()
{
	// 移動量
	const float speed = 0.03f;

	// --- 各操作方法の更新 ---
	UpdateGamePad();
	UpdateKeyboard();

	// --- 追従対象がいる時の更新 ---
	if (target_) {
		// 追従座標の補完
		targetPos_ = Lerp(targetPos_, target_->translation_, 0.2f);

		// 追従対象からのオフセット
		Vector3 offset = MakeOffset();
		// カメラ座標
		vp_.translation_ = targetPos_ + offset;
	}

	vp_.UpdateMatrix();
}

void FollowCamera::Reset()
{
	if (target_) {
		// 追従対象の初期化
		targetPos_ = target_->translation_;
		vp_.rotation_.y = target_->rotation_.y;
	}

	// 追従対象からのオフセット
	Vector3 offset = MakeOffset();
	vp_.translation_ = targetPos_ + offset;
}

void FollowCamera::SetTarget(const WorldTransform* target)
{
	target_ = target;
	Reset();
}

void FollowCamera::UpdateGamePad()
{
	//XINPUT_STATE joyState;
}

void FollowCamera::UpdateKeyboard()
{
	if (Input::GetInstance()->PushKey(DIK_LEFT)) {
		move += Vector3(0.0f, -1.0f, 0.0f);
	}
	if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
		move += Vector3(0.0f, 1.0f, 0.0f);
	}
}

Vector3 FollowCamera::MakeOffset()
{
	Matrix4x4 rotateMatrix = MakeAffineMatrix({ 1, 1, 1 }, vp_.rotation_, {});
	
	// カメラの回転に合わせて回転
	Vector3 offset = TransformNormal(offset_, rotateMatrix);
	return offset;
}
