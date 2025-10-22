#pragma once
#include "Sprite.h"
#include "memory"
#include "vector"

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
    /// セット
    /// </summary>
    void SetFadeInStart(bool start) { fadeInStart = start; }
    void SetFadeOutStart(bool start) { fadeOutStart = start; }
    void SetFadeInFinish(bool finish) { fadeInFinish = finish; }

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

  private:
    /// <summary>
    /// フェードイン（画面が黒く埋まる）
    /// </summary>
    void FadeIn();

    /// <summary>
    /// フェードアウト（次シーンへ移行）
    /// </summary>
    void FadeOut();

    /// <summary>
    /// グリッド矩形の初期化
    /// </summary>
    void InitializeGrid();

    /// <summary>
    /// グリッド矩形の更新
    /// </summary>
    void UpdateGrid();

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

    // フラグ
    bool fadeInStart = false;
    bool fadeOutStart = false;
    bool fadeInFinish = false;
    bool fadeOutFinish = false;
    bool isEnd = false;
};