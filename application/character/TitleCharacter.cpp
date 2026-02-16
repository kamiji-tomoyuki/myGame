#include "TitleCharacter.h"
#include "myMath.h"
#include "Easing.h"

void TitleCharacter::Init() {
    BaseObject::Init();
    BaseObject::SetScale({1.5f, 1.5f, 1.5f});
    BaseObject::SetWorldPositionX(3.7f);

    // --- モデルの初期化 ---
    obj3d_ = std::make_unique<Object3d>();
    obj3d_->Initialize("Title/PlayerBody.obj");
}

void TitleCharacter::Update() {
    if (timer_ < 0.0f || timer_ > 1.0f) {
        add_ *= -1.0f;
    }
    timer_ += add_;

    BaseObject::SetRotationY(BaseObject::GetTransform().rotation_.y + 0.01f);
    BaseObject::SetWorldPositionY(EaseInOutQuint(0.2f,-0.2f,timer_,1.0f));
    BaseObject::Update();
}

void TitleCharacter::Draw(const ViewProjection &viewProjection) {
    obj3d_->Draw(BaseObject::GetWorldTransform(), viewProjection);
}
