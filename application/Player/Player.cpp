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
	obj3d_->Initialize("Player/player.gltf");

	// --- 各パーツの初期化・設定 ---
	InitArm();

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

		// 攻撃処理の更新
		UpdateAttack();
	}

	// アニメーションの再生
	obj3d_->AnimationUpdate(true);

	// 腕の更新
	for (const std::unique_ptr<PlayerArm>& arm : arms_) {
		arm->Update();
	}

#ifdef _DEBUG
	hitEffect_->Update(*vp_);
#endif // _DEBUG
}

void Player::UpdateAttack()
{
	// 攻撃キー入力チェック（スペースキーで攻撃）
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {

		// 現在攻撃中でない場合、右腕で攻撃開始
		bool anyAttacking = false;
		for (const std::unique_ptr<PlayerArm>& arm : arms_) {
			if (arm->GetBehavior() == PlayerArm::Behavior::kAttack) {
				anyAttacking = true;
				break;
			}
		}

		if (!anyAttacking) {
			// 右腕で右パンチ開始
			if (arms_[kRArm]) {
				arms_[kRArm]->StartAttack(PlayerArm::AttackType::kRightPunch);
			}
		}
		else {
			// 左腕でコンボ攻撃（左パンチ）
			if (arms_[kLArm] && arms_[kLArm]->CanCombo()) {
				arms_[kLArm]->StartAttack(PlayerArm::AttackType::kLeftPunch);
			}
		}
	}

	// 右パンチ終了時に左腕にコンボタイマーを設定
	if (arms_[kRArm] && arms_[kRArm]->GetLastAttackType() == PlayerArm::AttackType::kRightPunch &&
		arms_[kRArm]->GetBehavior() == PlayerArm::Behavior::kNormal) {
		// 右パンチが終了した瞬間に左腕にコンボタイマーを設定
		if (arms_[kLArm]) {
			arms_[kLArm]->SetComboTimer(60); // 60フレームのコンボ受付時間
		}
	}
}

void Player::Draw(const ViewProjection& viewProjection)
{

}

void Player::DrawAnimation(const ViewProjection& viewProjection)
{
	if (isAlive_) {
		obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection);
		for (const std::unique_ptr<PlayerArm>& arm : arms_) {
			arm->DrawAnimation(viewProjection);
		}
	}
}

void Player::DrawParticle(const ViewProjection& viewProjection)
{
	hitEffect_->Draw(Ring);
}

void Player::ImGui()
{
	hitEffect_->imgui();

	// 腕のデバッグ情報表示
	for (size_t i = 0; i < arms_.size(); ++i) {
		if (arms_[i]) {
			arms_[i]->ImGui();
		}
	}

	// プレイヤーのデバッグ情報
	ImGui::Begin("Player Debug");
	ImGui::Text("Player Rotation Y: %.2f", BaseObject::GetTransform().rotation_.y);
	ImGui::Text("Player Position: (%.2f, %.2f, %.2f)",
		BaseObject::GetWorldPosition().x,
		BaseObject::GetWorldPosition().y,
		BaseObject::GetWorldPosition().z);

	// 攻撃状態の表示
	if (arms_[kRArm]) {
		ImGui::Text("Right Arm State: %s",
			arms_[kRArm]->GetBehavior() == PlayerArm::Behavior::kAttack ? "Attacking" : "Normal");
	}
	if (arms_[kLArm]) {
		ImGui::Text("Left Arm State: %s",
			arms_[kLArm]->GetBehavior() == PlayerArm::Behavior::kAttack ? "Attacking" : "Normal");
		ImGui::Text("Left Arm Can Combo: %s",
			arms_[kLArm]->CanCombo() ? "Yes" : "No");
	}

	ImGui::End();
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

void Player::InitArm()
{
	for (std::unique_ptr<PlayerArm>& arm : arms_) {
		arm = std::make_unique<PlayerArm>();
		arm->SetPlayer(this);
	}

	// 右腕の設定
	arms_[kRArm]->Init("player/Arm/playerArm.gltf");
	arms_[kRArm]->SetID(serialNumber_);
	arms_[kRArm]->SetIsRightArm(true);  // 右腕として設定
	arms_[kRArm]->SetTranslation(Vector3(-1.7f, 0.0f, 1.3f));
	arms_[kRArm]->SetScale(Vector3(0.8f, 0.8f, 0.8f));

	// 左腕の設定
	arms_[kLArm]->Init("player/Arm/playerArm.gltf");
	arms_[kLArm]->SetID(serialNumber_);
	arms_[kLArm]->SetIsRightArm(false); // 左腕として設定
	arms_[kLArm]->SetTranslation(Vector3(1.7f, 0.0f, 1.3f));
	arms_[kLArm]->SetScale(Vector3(0.8f, 0.8f, 0.8f));
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