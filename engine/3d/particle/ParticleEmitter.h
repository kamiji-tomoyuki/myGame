#pragma once
#include "ParticleManager.h"
#include "GlobalVariables.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include <string>

#ifdef _DEBUG
#include "imgui.h"
#endif // _DEBUG

class ParticleEmitter {
public:
    ParticleEmitter();

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="name">: パーティクル名</param>
    /// <param name="fileName">: オブジェクトのファイルパス</param>
    void Initialize(const std::string& name, const std::string& fileName);

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update(const ViewProjection& vp_);
    void UpdateOnce(const ViewProjection& vp_);

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw();

    /// <summary>
    /// エミッター描画処理
    /// </summary>
    void DrawEmitter();

    /// <summary>
    /// imgui表示
    /// </summary>
    void imgui();

public:

    /// 各ステータス設定関数
    /// <returns></returns>
    void SetPosition(const Vector3& position) { transform_.translation_ = position; }
    void SetScale(const Vector3& scale) { transform_.scale_ = scale; }
    void SetCount(const int& count) { count_ = count; }
    void SetActive(bool isActive) { isActive_ = isActive; }
    void SetValue();

private:
    /// <summary>
    /// パーティクル発生
    /// </summary>
    void Emit();

    void ApplyGlobalVariables();
    void AddItem();

    // --- パーティクルステータス ---
    std::string name_;          
    WorldTransform transform_; 
    int count_; 

    float emitFrequency_;
    float elapsedTime_; 

    float lifeTimeMin_;         
    float lifeTimeMax_; 
    float alphaMin_;
    float alphaMax_;
    float deltaTime = 1.0f / 60.0f;
    float scaleMin;
    float scaleMax;

    Vector3 velocityMin_;
    Vector3 velocityMax_;
    Vector3 startScale_;
    Vector3 endScale_;
    Vector3 startAcce_;
    Vector3 endAcce_;
    Vector3 startRote_ = {};
    Vector3 endRote_ = {};
    Vector3 rotateVelocityMin;
    Vector3 rotateVelocityMax;
    Vector3 allScaleMin;
    Vector3 allScaleMax;

    bool isRandomScale = false;
    bool isAllRamdomScale = false;
    bool isRandomColor = true;
    bool isRandomRotate = false;
    bool isVisible;
    bool isBillBoard = true;
    bool isActive_ = false;
    bool isAcceMultiply = false;
    bool isSinMove = false;

    std::unique_ptr<ParticleManager> manager_;

    GlobalVariables* globalVariables = nullptr;
    const char* groupName = nullptr;
};