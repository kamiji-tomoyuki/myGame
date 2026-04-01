#include "FollowCamera.h"
#include <Xinput.h>
#include <Input.h>

#include "myMath.h"
#include "Easing.h"

void FollowCamera::Initialize()
{
	vp_.Initialize();

	destinationAngle = Quaternion::IdentityQuaternion();

	vp_.translation_ = kInitialTranslation;
	targetPos_ = { 0.0f, 0.0f, 0.0f };
}

void FollowCamera::Update()
{
	// カメラが固定されている場合は、固定された位置・回転を維持
	if (isCameraFixed_) {
		vp_.translation_ = fixedPosition_;
		vp_.rotation_ = fixedRotation_;
		destinationAngle = fixedQuaternion_;
		vp_.UpdateMatrix();
		return;
	}

	// 以下は通常の追従処理
	UpdateGamePad();
	UpdateKeyboard();

	// Y角度はフィニッシャー中かどうかで使う値を切り替える
	float resolvedAngleY = ResolveDestinationAngleY();

	destinationAngle = Quaternion::Sleap(
		destinationAngle,
		Quaternion::MakeRotateAxisAngleQuaternion({ 0, 0, -1 }, destinationAngleX_) *
		Quaternion::MakeRotateAxisAngleQuaternion({ 0, -1, 0 }, resolvedAngleY),
		kRotationSlerpSpeed);
	vp_.rotation_ = destinationAngle.ToEulerAngles();

	if (target_) {
		if (isStartMove_) {
			startMoveTime_++;
			float t = min(startMoveTime_ / startMoveDuration_, 1.0f);

			vp_.translation_ = EaseOutSine(startPos_, targetStartPos_, t, 1.0f);

			if (t >= 1.0f) {
				isStartMove_ = false;
				targetPos_ = target_->translation_;
			}
		}
		else {
			// 通常の追従
			targetPos_ = Lerp(targetPos_, target_->translation_, kPositionLerpSpeed);
			Vector3 offset = MakeOffset();
			vp_.translation_ = targetPos_ + offset;
		}
	}
	else {
		vp_.translation_ = targetPos_;
	}

	vp_.UpdateMatrix();
}

float FollowCamera::ResolveDestinationAngleY() const
{
	// フィニッシャー中・ロックオン中は stableAngleY_ を優先
	if (hasStableAngle_) {
		return stableAngleY_;
	}

	// 通常時はプレイヤー回転に追従
	if (target_) {
		return target_->rotation_.y;
	}

	return destinationAngleY_;
}

void FollowCamera::Reset()
{
	if (target_) {
		targetPos_ = target_->translation_;
		vp_.rotation_.y = target_->rotation_.y;
		destinationAngleY_ = vp_.rotation_.y;
		stableAngleY_ = destinationAngleY_;

		Vector3 offset = MakeOffset();
		vp_.translation_ = targetPos_ + offset;
	}
	else {
		targetPos_ = { 0.0f, 0.0f, 0.0f };
		vp_.rotation_ = { destinationAngleX_, destinationAngleY_, 0.0f };
		destinationAngle = Quaternion::FromEulerAngles(vp_.rotation_);
		stableAngleY_ = destinationAngleY_;

		Vector3 offset = MakeOffset();
		vp_.translation_ = targetPos_ + offset;
	}
}

void FollowCamera::SetTarget(const WorldTransform* target)
{
	target_ = target;
}

void FollowCamera::StartFollowMove()
{
	if (!target_) return;

	isStartMove_ = true;
	startMoveTime_ = 0.0f;
	startPos_ = vp_.translation_;

	Vector3 followOffset = MakeOffset();
	targetStartPos_ = target_->translation_ + followOffset;
}

void FollowCamera::SetCameraFixed(bool isFixed)
{
	isCameraFixed_ = isFixed;

	if (isFixed) {
		if (target_) {
			targetPos_ = target_->translation_;

			destinationAngleY_ = target_->rotation_.y;
			stableAngleY_ = destinationAngleY_;
			destinationAngle =
				Quaternion::MakeRotateAxisAngleQuaternion({ 0, 0, -1 }, destinationAngleX_) *
				Quaternion::MakeRotateAxisAngleQuaternion({ 0, -1, 0 }, destinationAngleY_);
			vp_.rotation_ = destinationAngle.ToEulerAngles();

			Vector3 offset = MakeOffset();
			vp_.translation_ = targetPos_ + offset;
			vp_.UpdateMatrix();
		}

		fixedPosition_ = vp_.translation_;
		fixedRotation_ = vp_.rotation_;
		fixedQuaternion_ = destinationAngle;
	}
}

void FollowCamera::UpdateGamePad()
{
	// XINPUT_STATE joyState;
}

void FollowCamera::UpdateKeyboard()
{
	if (Input::GetInstance()->PushKey(DIK_LEFT)) {
		move += Vector3(0.0f, -kKeyboardRotateSpeed, 0.0f);
	}
	if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
		move += Vector3(0.0f, kKeyboardRotateSpeed, 0.0f);
	}
}

Vector3 FollowCamera::MakeOffset()
{
	if (!target_) {
		return Vector3(0.0f, 0.0f, 0.0f);
	}

	Matrix4x4 rotateMatrix = MakeAffineMatrix({ 1, 1, 1 }, vp_.rotation_, {});
	Vector3 offset = TransformNormal(offset_, rotateMatrix);
	return offset;
}