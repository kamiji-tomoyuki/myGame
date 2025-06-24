#include "Player.h"
#include "Input.h"

#include "FollowCamera.h"
#include "CollisionTypeIdDef.h"

#include "myMath.h"
#include <Enemy.h>

Player::Player()
{
	serialNumber_ = nextSerialNumber_;
	nextSerialNumber_++;
}

void Player::Init()
{
	BaseObject::Init();
	Collider::SetTypeID(static_cast<uint32_t>(CollisionTypeIdDef::kPlayer));

	// --- ステージマネージャー ---
	stageManager_ = StageManager::GetInstance();
	stageManager_->Initialize();

	// 初期位置設定
	Vector3 initialPos = { 0.0f, 0.0f, -15.0f };
	BaseObject::SetWorldPosition(initialPos);

	// --- モデルの初期化 ---
	obj3d_ = std::make_unique<Object3d>();
	obj3d_->Initialize("walk.gltf");

	// --- 各ステータスの初期値設定 ---
	isMove_ = true;


	// --- 各エフェクト・演出の初期設定 ---
	hitEffect_ = std::make_unique<ParticleEmitter>();
	hitEffect_->Initialize("hitEffect", "debug/ringPlane.obj");
}

void Player::Update()
{
	BaseObject::Update();

	if (isAlive_) {
		// 移動
		hitEffect_->SetPosition(BaseObject::GetWorldPosition());
		Move();
	}

	// アニメーションの再生
	obj3d_->AnimationUpdate(true);

#ifdef _DEBUG
	hitEffect_->Update(*vp_);
#endif // _DEBUG

}

void Player::Draw(const ViewProjection& viewProjection)
{

}

void Player::DrawAnimation(const ViewProjection& viewProjection)
{
	if (isAlive_) {
		obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection);
	}
}

void Player::DrawParticle(const ViewProjection& viewProjection)
{
	hitEffect_->Draw(Ring);
}

void Player::ImGui()
{
	hitEffect_->imgui();
}

void Player::OnCollision(Collider* other)
{
	uint32_t typeID = other->GetTypeID();

	//衝突相手
	if (typeID == static_cast<uint32_t>(CollisionTypeIdDef::kEnemy)) {
		Enemy* enemy = static_cast<Enemy*>(other);

		// 重複処理を避けるため、シリアル番号の大きい方で処理
		if (GetSerialNumber() > enemy->GetSerialNumber()) {
			return;
		}

		// 仮処理
		//isAlive_ = false;
		hitEffect_->UpdateOnce(*vp_);

		// 衝突処理後もステージ境界内に留める
		Vector3 playerPos = BaseObject::GetWorldPosition();
		if (stageManager_ != nullptr && !stageManager_->IsWithinStageBounds(playerPos)) {
			playerPos = stageManager_->ClampToStageBounds(playerPos);
			BaseObject::SetWorldPosition(playerPos);
		}
	}

	transform_.UpdateMatrix();
}

void Player::Move()
{
	Vector3 move = { 0.0f, 0.0f, 0.0f };

	// --- キーボード ---
	if (Input::GetInstance()->PushKey(DIK_D)) {
		move.x += kAcceleration_;
	}
	if (Input::GetInstance()->PushKey(DIK_A)) {
		move.x -= kAcceleration_;
	}
	if (Input::GetInstance()->PushKey(DIK_W)) {
		move.z += kAcceleration_;
	}
	if (Input::GetInstance()->PushKey(DIK_S)) {
		move.z -= kAcceleration_;
	}

	// 回転
	if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
		BaseObject::SetRotationY(BaseObject::GetTransform().rotation_.y + kRotateAcceleration_);
	}
	if (Input::GetInstance()->PushKey(DIK_LEFT)) {
		BaseObject::SetRotationY(BaseObject::GetTransform().rotation_.y - kRotateAcceleration_);
	}

	// 入力ベクトルがある場合
	if (move.Length() != 0.0f) {
		move.Normalize();  // 正規化して速度が一定になるようにする

		// rotationY の角度に基づく回転行列
		float rotY = BaseObject::GetTransform().rotation_.y;
		Matrix4x4 rotMat = MakeRotateYMatrix(rotY);

		// 向いている方向に移動ベクトルを回転
		move = TransformNormal(move, rotMat);

		velocity_ += move * kAcceleration_;  // 加速度をかけて速度に反映
	}
	else {
		velocity_ = 0.0f;
	}

	// 速度上限
	if (velocity_.Length() > kMaxSpeed_) {
		velocity_ = velocity_.Normalize() * kMaxSpeed_;
	}

	// 位置の更新（境界チェック付き）
	Vector3 currentPos = BaseObject::GetWorldPosition();
	Vector3 newPos = currentPos + velocity_;

	// ステージマネージャーが正常に初期化されているかチェック
	if (stageManager_ != nullptr) {
		// ステージ境界チェック
		if (!stageManager_->IsWithinStageBounds(newPos)) {
			// 境界外の場合は補正
			newPos = stageManager_->ClampToStageBounds(newPos);

			// 境界に衝突した場合は速度を減衰させる（完全にリセットしない）
			velocity_ *= 0.5f;
		}
	}

	BaseObject::SetWorldPosition(newPos);

	// --- ゲームパッド ---
}