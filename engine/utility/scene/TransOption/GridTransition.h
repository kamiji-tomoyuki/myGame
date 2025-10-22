#pragma once
#include "Sprite.h"
#include "memory"
#include "vector"

class GridTransition {
  public:
    GridTransition();
    ~GridTransition();

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
    /// セッター
    /// </summary>
    void SetTransitionStart(bool start) { transitionStart_ = start; }
    void SetDuration(float duration) { duration_ = duration; }
    void SetIsCloseTransition(bool isClose) { isCloseTransition_ = isClose; }
    void SetGridSize(int cols, int rows) {
        gridCols_ = cols;
        gridRows_ = rows;
    }

    /// <summary>
    /// ゲッター
    /// </summary>
    bool IsEnd() const { return isEnd_; }
    bool IsTransitioning() const { return transitionStart_ && !isEnd_; }

    /// <summary>
    /// リセット
    /// </summary>
    void Reset();

  private:
    /// <summary>
    /// グリッド矩形の初期化
    /// </summary>
    void InitializeGridRects();

    /// <summary>
    /// トランジション処理
    /// </summary>
    void ProcessTransition();

    /// <summary>
    /// 各矩形の更新
    /// </summary>
    void UpdateGridRects();

  private:
    // グリッド構造体
    struct GridRect {
        std::unique_ptr<Sprite> sprite;
        Vector2 position;
        Vector2 size;
        int waveIndex;      // 波の順番（チェッカーボードパターン用）
        float delayTime;    // 表示開始の遅延時間
        float currentAlpha; // 現在のアルファ値
    };

    // フェードの持続時間
    float duration_ = 1.0f;
    // 経過時間カウンター
    float counter_ = 0.0f;
    // 現在の進行度 (0.0f ~ 1.0f)
    float progress_ = 0.0f;

    // グリッドの列数と行数
    int gridCols_ = 16;
    int gridRows_ = 9;

    // 画面サイズ
    const float screenWidth_ = 1280.0f;
    const float screenHeight_ = 720.0f;

    // グリッド矩形の配列
    std::vector<GridRect> gridRects_;

    // フラグ
    bool transitionStart_ = false;
    bool isEnd_ = false;
    // true: 閉じる（黒く埋まる）, false: 開く（次シーンへ）
    bool isCloseTransition_ = false;

    // 波の総数（チェッカーボードパターンの段階数）
    int totalWaves_ = 0;
};