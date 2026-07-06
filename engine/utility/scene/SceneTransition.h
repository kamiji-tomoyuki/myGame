#pragma once
#include "Sprite.h"
#include "memory"
#include "vector"
#include "ISceneTransitionState.h"

namespace Engine {
class SceneTransition {
  public:
    SceneTransition();
    ~SceneTransition();

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

    /// <summary>
    /// 状態変更
    /// </summary>
    /// <param name="newState"></param>
    void ChangeState(std::unique_ptr<ISceneTransitionState> newState);

    /// <summary>
    /// セット
    /// </summary>
    void SetFadeInStart(bool start);
    void SetFadeOutStart(bool start);
    void SetFadeInFinish(bool finish);

    /// <summary>
    /// getter
    /// </summary>
    bool IsEnd() { return isEnd; }
    bool FadeInFinish() { return fadeInFinish; }
    bool FadeInStart() { return fadeInStart; }

    /// <summary>
    /// リセット
    /// </summary>
    void Reset();

    /// <summary>
    /// グリッドサイズ設定（オプション）
    /// </summary>
    void SetGridSize(int cols, int rows) {
        gridCols_ = cols;
        gridRows_ = rows;
        InitializeGrid();
    }

  public:
    /// <summary>
    /// フェードイン処理（内部用）
    /// </summary>
    void ProcFadeIn();

    /// <summary>
    /// フェードアウト処理（内部用）
    /// </summary>
    void ProcFadeOut();

    /// <summary>
    /// グリッド矩形の更新（内部用）
    /// </summary>
    void UpdateGrid();

    /// <summary>
    /// 全てのグリッドを描画
    /// </summary>
    void DrawGrid();

  private:
    /// <summary>
    /// グリッド矩形の初期化
    /// </summary>
    void InitializeGrid();

  private:
    // グリッド構造体
    struct GridRect {
        std::unique_ptr<Sprite> sprite;
        Vector2 position;
        Vector2 size;
        int waveIndex;
        float delayTime;
        float currentAlpha;
        float fadeStartTime;
    };

    // 現在の状態
    std::unique_ptr<ISceneTransitionState> state_;

    // フェードの持続時間
    float duration_ = 0.0f;
    // 経過時間カウンター
    float counter_ = 0.0f;

    // グリッド設定
    int gridCols_ = 16;
    int gridRows_ = 9;
    const float screenWidth_ = 1280.0f;
    const float screenHeight_ = 720.0f;
    int totalWaves_ = 0;

    // グリッド矩形の配列
    std::vector<GridRect> gridRects_;

    // フラグ (State Pattern移行後も互換性のために残すが、状態管理はstate_で行う)
    bool fadeInStart = false;
    bool fadeOutStart = false;
    bool fadeInFinish = false;
    bool fadeOutFinish = false;
    bool isEnd = false;
};
} // namespace Engine
