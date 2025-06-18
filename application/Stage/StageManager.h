#pragma once
#include "myMath.h"

/// <summary>
/// ステージ管理クラス
/// </summary>
class StageManager
{
public:
    /// <summary>
    /// インスタンス取得
    /// </summary>
    /// <returns>StageManagerのインスタンス</returns>
    static StageManager* GetInstance();

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 指定座標がステージ境界内かどうかを判定
    /// </summary>
    /// <param name="position">判定する座標</param>
    /// <returns>境界内ならtrue</returns>
    bool IsWithinStageBounds(const Vector3& position) const;

    /// <summary>
    /// 座標をステージ境界内に補正
    /// </summary>
    /// <param name="position">補正する座標</param>
    /// <returns>補正後の座標</returns>
    Vector3 ClampToStageBounds(const Vector3& position) const;

    /// <summary>
    /// ステージ中心座標の取得
    /// </summary>
    /// <returns>ステージ中心座標</returns>
    const Vector3& GetStageCenter() const { return stageCenter_; }

    /// <summary>
    /// ステージ半径の取得
    /// </summary>
    /// <returns>ステージ半径</returns>
    float GetStageRadius() const { return stageRadius_; }

    /// <summary>
    /// ステージ中心座標の設定
    /// </summary>
    /// <param name="center">新しい中心座標</param>
    void SetStageCenter(const Vector3& center) { stageCenter_ = center; }

    /// <summary>
    /// ステージ半径の設定
    /// </summary>
    /// <param name="radius">新しい半径</param>
    void SetStageRadius(float radius) { stageRadius_ = radius; }

    /// <summary>
    /// ステージからの距離を取得
    /// </summary>
    /// <param name="position">対象座標</param>
    /// <returns>ステージ中心からの距離</returns>
    float GetDistanceFromCenter(const Vector3& position) const;

    /// <summary>
    /// ステージ境界までの距離を取得
    /// </summary>
    /// <param name="position">対象座標</param>
    /// <returns>境界までの距離（負の値は境界外を示す）</returns>
    float GetDistanceFromBoundary(const Vector3& position) const;

private:
    StageManager() = default;
    ~StageManager() = default;
    StageManager(const StageManager&) = delete;
    StageManager& operator=(const StageManager&) = delete;

    // ステージ設定
    Vector3 stageCenter_;   // ステージの中心座標
    float stageRadius_;     // ステージの半径
};