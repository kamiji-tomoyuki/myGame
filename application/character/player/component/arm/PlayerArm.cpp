#include "PlayerArm.h"
#include "Input.h"
#include "myMath.h"
#include "Player.h"
#include "PlayerAttack.h"
#include <Enemy.h>

// =============================================================
//  関数ポインタテーブルの定義
// =============================================================
using namespace Engine;
const std::unordered_map<PlayerArm::Behavior, PlayerArm::BehaviorFunc>
PlayerArm::kBehaviorTable_ = {
    { Behavior::kAttack, &PlayerArm::UpdateAttackBehavior },
    { Behavior::kRush,   &PlayerArm::UpdateRushBehavior   },
};

PlayerArm::PlayerArm()
{
    serialNumber_ = nextSerialNumber_;
    nextSerialNumber_++;
}

void PlayerArm::Init(const std::string& filePath)
{
    BaseObject::Init();
    Collider::Initialize();

    obj3d_ = std::make_unique<Object3d>();
    obj3d_->Initialize(filePath);

    attack_ = std::make_unique<PlayerArmAttack>();
    rush_ = std::make_unique<PlayerArmRush>();

    attack_->SetAttackDamage(kInitAttackDamage_);
    rush_->SetRushAttackDamage(kInitRushAttackDamage_);
    rush_->SetFinisherAttackDamage(kInitFinisherAttackDamage_);

    Collider::SetRadius(kColliderRadius_);
    Collider::SetCollisionEnabled(true);
}

// =============================================================
//  更新
// =============================================================
void PlayerArm::Update()
{
    BaseObject::Update();

    auto it = kBehaviorTable_.find(behavior_);
    if (it != kBehaviorTable_.end()) {
        bool finished = (this->*(it->second))();
        if (finished) {
            behavior_ = Behavior::kNormal;
        }
    }

    attack_->UpdateComboTimer();

    // Behavior 更新で transform_.translation_ を書き換えた後、
    // コライダーが参照するワールド行列を再計算する。
    // （BaseObject::Update() は Behavior より前に呼ばれるため行列が古くなる）
    transform_.UpdateMatrix();

    obj3d_->UpdateAnimation(true);
    Collider::UpdateWorldTransform();
}

// =============================================================
//  Behavior 更新関数 — 攻撃
// =============================================================
bool PlayerArm::UpdateAttackBehavior()
{
    bool finished = attack_->Update();
    transform_.translation_ = attack_->GetCurrentTranslation();
    return finished;
}

// =============================================================
//  Behavior 更新関数 — ラッシュ
// =============================================================
bool PlayerArm::UpdateRushBehavior()
{
    float bodyRotY = 0.0f;
    if (transform_.parent_ != nullptr) {
        bodyRotY = transform_.parent_->rotation_.y;
    }

    bool finished = rush_->Update(bodyRotY);
    transform_.translation_ = rush_->GetCurrentTranslation();
    transform_.rotation_ = rush_->GetCurrentRotation();

    if (finished) {
        transform_.rotation_ = { 0.0f, 0.0f, 0.0f };
        attack_->SetComboCount(0);
        attack_->SetComboTimer(0);
        attack_->SetLastAttackType(AttackType::kNone);
    }

    return finished;
}

// =============================================================
//  攻撃開始
// =============================================================
void PlayerArm::StartAttack(AttackType attackType)
{
    if (behavior_ == Behavior::kAttack || behavior_ == Behavior::kRush) { return; }

    behavior_ = Behavior::kAttack;
    attack_->StartAttack(attackType, isRightArm_, transform_.translation_);
}

// =============================================================
//  ラッシュ開始
//  timerOffset — PlayerArmRush::rushTimer_ の初期値。
//  左右の腕で kRushInterval/2 フレームずらすことで交互パンチを実現する。
// =============================================================
void PlayerArm::StartRush(uint32_t timerOffset)
{
    if (behavior_ == Behavior::kAttack || behavior_ == Behavior::kRush) { return; }

    behavior_ = Behavior::kRush;
    rush_->StartRush(isRightArm_, transform_.translation_, timerOffset);
}

bool PlayerArm::CanCombo() const
{
    return (attack_->CanCombo() && behavior_ == Behavior::kNormal);
}

bool PlayerArm::CanStartRush() const
{
    return (behavior_ == Behavior::kNormal && attack_->CanStartRush());
}

// =============================================================
//  描画
// =============================================================
void PlayerArm::Draw(const ViewProjection& viewProjection) {}

void PlayerArm::DrawAnimation(const ViewProjection& viewProjection)
{
    // objColor_ を渡してアルファを反映（残像腕の半透明表現に使用。本体腕は既定=不透明）
    obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection, &objColor_);
}

void PlayerArm::DrawParticle(const ViewProjection& viewProjection) {}

// =============================================================
//  HandleHit（OnCollision / OnCollisionEnter の共通処理）
// =============================================================
void PlayerArm::HandleHit(Collider* other)
{
    if (!attack_->GetIsAttack() && !rush_->GetIsRush()) { return; }

    uint32_t typeID = other->GetTypeID();

    if (typeID == static_cast<uint32_t>(CollisionTypeIdDef::kEnemy)) {
        Enemy* enemy = static_cast<Enemy*>(other);

        // -------------------------------------------------------
        // フィニッシャー判定（右腕専用）
        // -------------------------------------------------------
        if (rush_->GetIsRush() && isRightArm_ && !rush_->HasFinisherHit()) {

            bool isFinisherOrEarlyRecover =
                (rush_->GetRushPhase() == RushPhase::kFinisher ||
                    rush_->GetRushPhase() == RushPhase::kRecover);

            if (isFinisherOrEarlyRecover && rush_->IsFinisherHitFrame()) {
                enemy->TakeDamage(rush_->GetFinisherAttackDamage());
                enemy->OnRushHit(true);
                rush_->SetHasFinisherHit(true);
                rush_->SetIsFinisherHitFrame(false);

                if (playerAttack_) {
                    playerAttack_->OnHit(PlayerUltGauge::HitType::kFinisher);
                }
            }
        }
        // -------------------------------------------------------
        // 連続パンチ判定
        // -------------------------------------------------------
        else if (rush_->GetIsRush() &&
            rush_->GetRushPhase() == RushPhase::kRapidPunch && rush_->IsRushAttackActive()) {
            uint32_t rushAttackTimer = rush_->GetRushAttackTimer();
            if (rushAttackTimer >= kRushHitTimerMin_ && rushAttackTimer <= kRushHitTimerMax_) {
                int currentFrame = static_cast<int>(rush_->GetRushTimer());
                if (currentFrame - rush_->GetLastRushHitFrame() >= kRushHitInterval_) {
                    enemy->TakeDamage(rush_->GetRushAttackDamage());
                    enemy->OnRushHit(false);
                    rush_->SetLastRushHitFrame(currentFrame);

                    if (playerAttack_) {
                        playerAttack_->OnHit(PlayerUltGauge::HitType::kRush);
                    }
                }
            }
        }
        // -------------------------------------------------------
        // 通常攻撃判定
        // -------------------------------------------------------
        else if (attack_->GetIsAttack()) {
            float progress = attack_->GetAttackProgress();
            if (progress >= kAttackHitProgressMin_ && progress <= kAttackHitProgressMax_ && !attack_->GetHasHitThisAttack()) {
                enemy->TakeDamage(attack_->GetAttackDamage());
                attack_->SetHasHitThisAttack(true);

                if (playerAttack_) {
                    PlayerUltGauge::HitType hitType = isRightArm_
                        ? PlayerUltGauge::HitType::kRightPunch
                        : PlayerUltGauge::HitType::kLeftPunch;
                    playerAttack_->OnHit(hitType);
                }
            }
        }
    }
}

// =============================================================
//  OnCollisionEnter
// =============================================================
void PlayerArm::OnCollisionEnter(Collider* other)
{
    HandleHit(other);
}

// =============================================================
//  OnCollision
// =============================================================
void PlayerArm::OnCollision(Collider* other)
{
    HandleHit(other);
}

// =============================================================
//  SetPlayer
// =============================================================
void PlayerArm::SetPlayer(Player* player)
{
    player_ = player;
    BaseObject::transform_.parent_ = &player->GetWorldTransform();
}