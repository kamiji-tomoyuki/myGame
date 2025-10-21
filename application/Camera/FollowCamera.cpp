#include "FollowCamera.h"
#include <Xinput.h>
#include <Input.h>

#include "myMath.h"
#include "Easing.h"

void FollowCamera::Initialize()
{
	vp_.Initialize();

	destinationAngle = Quaternion::IdentityQuaternion();

	// 初期カメラ位置を設定
	vp_.translation_ = { 0.0f, 2.0f, -10.0f };
	targetPos_ = { 0.0f, 0.0f, 0.0f };
}

void FollowCamera::Update()
{
	UpdateGamePad();
	UpdateKeyboard();

	destinationAngle = Quaternion::Sleap(destinationAngle,
		Quaternion::MakeRotateAxisAngleQuaternion({ 0,0,-1 }, destinationAngleX_) *
		Quaternion::MakeRotateAxisAngleQuaternion({ 0,-1,0 }, destinationAngleY_), 0.1f);
	vp_.rotation_ = destinationAngle.ToEulerAngles();

	if (target_) {
		if (isStartMove_) {
			startMoveTime_++;
			float t = min(startMoveTime_ / startMoveDuration_, 1.0f);

			// イージング補間
			vp_.translation_ = EaseOutSine(startPos_, targetStartPos_, t, 1.0f);

			if (t >= 1.0f) {
				isStartMove_ = false;
				// 完全に追従位置に切り替え
				targetPos_ = target_->translation_;
			}
		}
		else {
			// 通常の追従
			targetPos_ = Lerp(targetPos_, target_->translation_, 0.1f);
			Vector3 offset = MakeOffset();
			vp_.translation_ = targetPos_ + offset;
		}
	}
	else {
		vp_.translation_ = targetPos_;
	}

	vp_.UpdateMatrix();
}

void FollowCamera::Reset()
{
	if (target_) {
		// 追従対象の初期化
		targetPos_ = target_->translation_;
		vp_.rotation_.y = target_->rotation_.y;
		destinationAngleY_ = vp_.rotation_.y;

		// 追従対象からのオフセット
		Vector3 offset = MakeOffset();
		vp_.translation_ = targetPos_ + offset;
	}
	else {
		// 追従対象がない場合はデフォルト位置にリセット
		targetPos_ = { 0.0f, 0.0f, 0.0f };
		vp_.rotation_ = { destinationAngleX_, destinationAngleY_, 0.0f };
		destinationAngle = Quaternion::FromEulerAngles(vp_.rotation_);

		Vector3 offset = MakeOffset();
		vp_.translation_ = targetPos_ + offset;
	}
}

void FollowCamera::SetTarget(const WorldTransform* target)
{
	target_ = target;
	//Reset();
}

void FollowCamera::StartFollowMove()
{
	if (!target_) return;

	isStartMove_ = true;
	startMoveTime_ = 0.0f;
	startPos_ = vp_.translation_; // Initialize時のカメラ位置

	// プレイヤーを追従したときの理想カメラ位置を算出
	Vector3 followOffset = MakeOffset();
	targetStartPos_ = target_->translation_ + followOffset;
}

void FollowCamera::UpdateGamePad()
{
	//XINPUT_STATE joyState;
}

void FollowCamera::UpdateKeyboard()
{
	// 移動量
	const float speed = 0.03f;

	if (Input::GetInstance()->PushKey(DIK_LEFT)) {
		move += Vector3(0.0f, -1.0f, 0.0f);
	}
	if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
		move += Vector3(0.0f, 1.0f, 0.0f);
	}

	// 追従対象がある場合のみ、その回転を反映
	if (target_) {
		destinationAngleY_ = target_->rotation_.y;
	}
}

Vector3 FollowCamera::MakeOffset()
{
	// 追従対象がいない場合はオフセットを適用しない
	if (!target_) {
		return Vector3(0.0f, 0.0f, 0.0f);
	}
	 
	Matrix4x4 rotateMatrix = MakeAffineMatrix({ 1, 1, 1 }, vp_.rotation_, {});

	// カメラの回転に合わせて回転
	Vector3 offset = TransformNormal(offset_, rotateMatrix);
	return offset;
}