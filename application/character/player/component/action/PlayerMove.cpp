#include "PlayerMove.h"
#include "Input.h"
#include "myMath.h"
#include <StageManager.h>
#include <ParticleEmitter.h>

using namespace Engine;
void PlayerMove::Init(StageManager* stageManager)
{
	stageManager_ = stageManager;
	velocity_ = {};
	dodgeRequested_ = false;
	dodgeDirection_ = {};
	isTrailActive_ = false;
	lastTrailPosition_ = {};
}

// =============================================================
//  移動更新
//  戻り値: 回避方向（ゼロなら回避入力なし）
// =============================================================
Vector3 PlayerMove::Update(
	const Vector3& currentWorldPos,
	float          currentRotationY,
	Vector3& outNewPos,
	float& outNewRotY)
{
	dodgeRequested_ = false;
	dodgeDirection_ = {};

	Vector3 move = {};

	if (Input::GetInstance()->PushKey(DIK_D)) { move.x += kAcceleration; }
	if (Input::GetInstance()->PushKey(DIK_A)) { move.x -= kAcceleration; }
	if (Input::GetInstance()->PushKey(DIK_W)) { move.z += kAcceleration; }
	if (Input::GetInstance()->PushKey(DIK_S)) { move.z -= kAcceleration; }

	float newRotY = currentRotationY;
	if (Input::GetInstance()->PushKey(DIK_RIGHT)) { newRotY += kRotateAcceleration; }
	if (Input::GetInstance()->PushKey(DIK_LEFT)) { newRotY -= kRotateAcceleration; }
	outNewRotY = newRotY;

	// --- Shift + 移動 → 回避リクエスト ---
	if ((Input::GetInstance()->TriggerKey(DIK_LSHIFT) ||
		Input::GetInstance()->TriggerKey(DIK_RSHIFT)) &&
		move.Length() != 0.0f)
	{
		dodgeRequested_ = true;
		dodgeDirection_ = move.Normalize();
		outNewPos = currentWorldPos;
		return dodgeDirection_;
	}

	// --- 通常移動 ---
	if (move.Length() != 0.0f) {
		move = move.Normalize();
		Matrix4x4 rotMat = MakeRotateYMatrix(newRotY);
		move = TransformNormal(move, rotMat);
		velocity_ += move * kAcceleration;	// 正規化済み方向 * 加速度
	}
	else {
		velocity_ *= 0.8f;
	}

	if (velocity_.Length() > kMaxSpeed) {
		velocity_ = velocity_.Normalize() * kMaxSpeed;
	}

	Vector3 newPos = currentWorldPos + velocity_;

	if (stageManager_ != nullptr) {
		if (!stageManager_->IsWithinStageBounds(newPos)) {
			newPos = stageManager_->ClampToStageBounds(newPos);
			velocity_ *= 0.5f;
		}
	}

	outNewPos = newPos;

	UpdateTrailEffect(newPos);

	return {};	// 回避なし
}

// =============================================================
//  軌跡エフェクト更新
// =============================================================
void PlayerMove::UpdateTrailEffect(const Vector3& currentWorldPos)
{
	if (!trailEffect_) { return; }

	Vector3 footPosition = currentWorldPos;
	footPosition.y += kFootOffsetY_;

	if (velocity_.Length() > 0.01f) {
		trailEffect_->SetPosition(footPosition);
		trailEffect_->SetActive(true);
		isTrailActive_ = true;
	}
	else {
		trailEffect_->SetActive(false);
		isTrailActive_ = false;
	}
}