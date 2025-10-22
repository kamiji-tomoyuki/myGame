#pragma once
#include "application/Base/BaseObject.h"

class TitleCharacter : public BaseObject {
  public:
    /// <summary>
    /// 初期化
    /// </summary>
    void Init() override;

    /// <summary>
    /// 更新
    /// </summary>
    void Update() override;

    /// <summary>
    /// 描画
    /// </summary>
    void Draw(const ViewProjection &viewProjection) override;

  private:
    // --- モデル ---
    std::unique_ptr<Object3d> obj3d_;

    float timer_ = 0.0f;
    float add_ = 1.0f / 60.0f;
};
