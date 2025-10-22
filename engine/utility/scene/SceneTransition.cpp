#include "SceneTransition.h"
#include "TextureManager.h"
#include <algorithm>
#include <cmath>
#include <random>

SceneTransition::SceneTransition() {}

SceneTransition::~SceneTransition() {}

void SceneTransition::Initialize() {
    duration_ = 1.0f; // フェードの持続時間（例: 1秒）
    counter_ = 0.0f;  // 経過時間カウンターを初期化
    fadeInFinish = false;
    fadeOutFinish = false;
    fadeInStart = false;
    fadeOutStart = false;
    isEnd = false;

    // グリッドの初期化
    InitializeGrid();
    SetGridSize(12, 8);
}

void SceneTransition::InitializeGrid() {
    gridRects_.clear();

    // 各矩形のサイズを計算
    float rectWidth = screenWidth_ / gridCols_;
    float rectHeight = screenHeight_ / gridRows_;

    totalWaves_ = gridRows_ - 1;

    // ランダムな列の順序を生成
    std::vector<int> columnOrder;
    for (int col = 0; col < gridCols_; ++col) {
        columnOrder.push_back(col);
    }

    // シャッフル（ランダムな順序に並び替え）
    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(columnOrder.begin(), columnOrder.end(), rng);

    // 各列にランダムなオフセットを割り当て
    std::vector<float> columnOffsets(gridCols_);
    for (int i = 0; i < gridCols_; ++i) {
        // 0.0から0.3の範囲でランダムなオフセットを生成
        columnOffsets[columnOrder[i]] = static_cast<float>(i) / static_cast<float>(gridCols_ - 1) * 0.3f;
    }

    for (int row = 0; row < gridRows_; ++row) {
        for (int col = 0; col < gridCols_; ++col) {
            GridRect rect;

            // スプライト初期化
            rect.sprite = std::make_unique<Sprite>();
            rect.sprite->Initialize("white1x1.png", {0, 0});
            rect.sprite->SetColor(Vector3(0.0f, 0.0f, 0.0f));
            rect.sprite->SetAlpha(0.0f);

            // 位置とサイズ設定
            rect.position = Vector2(col * rectWidth, row * rectHeight);
            rect.size = Vector2(rectWidth, rectHeight);
            rect.sprite->SetPosition(rect.position);
            rect.sprite->SetSize(rect.size);

            // 上から下へのパターンで波のインデックスを設定
            rect.waveIndex = row;

            // 遅延時間を計算（行の遅延 + 列のランダムオフセット）
            float baseDelay = static_cast<float>(rect.waveIndex) / static_cast<float>(totalWaves_);
            rect.delayTime = baseDelay * 0.7f + columnOffsets[col]; // 行の影響を70%に調整
            rect.currentAlpha = 0.0f;
            rect.fadeStartTime = -1.0f; // まだフェード開始していない

            gridRects_.push_back(std::move(rect));
        }
    }
}

void SceneTransition::Update() {
    if (fadeInStart) {
        // フェードイン中（画面が黒く埋まる）
        if (!fadeInFinish) {
            FadeIn();
        }
    }
    if (fadeOutStart) {
        // フェードインが終わったら、フェードアウトを開始
        if (fadeInFinish && !fadeOutFinish) {
            FadeOut();
        }
    }

    // グリッドの更新
    UpdateGrid();

    // トランジションが終了したら、終了フラグを立てる
    if (fadeInFinish && fadeOutFinish) {
        isEnd = true;
        fadeInStart = false;
        fadeOutStart = false;
    }
}

void SceneTransition::Draw() {
    // 全てのグリッド矩形を描画
    for (auto &rect : gridRects_) {
        rect.sprite->Draw();
    }
}

void SceneTransition::FadeIn() {
    counter_ += 1.0f / 60.0f; // フレームレートを基にカウント

    // 全てのグリッドが完全に表示されるまで待つ
    // 最大遅延時間 + アニメーション時間を考慮
    float maxDelayTime = 0.0f;
    for (auto &rect : gridRects_) {
        maxDelayTime = max(maxDelayTime, rect.delayTime);
    }
    float maxDuration = maxDelayTime + duration_;

    if (counter_ >= maxDuration) {
        counter_ = maxDuration; // 終了時間を超えないように制限
        fadeInFinish = true;    // フェードイン終了フラグを立てる
    }
}

void SceneTransition::FadeOut() {
    // フェードアウト開始時に各矩形のfadeStartTimeをリセット
    static bool resetDone = false;
    if (!resetDone) {
        for (auto &rect : gridRects_) {
            rect.fadeStartTime = -1.0f;
        }
        resetDone = true;
    }

    // カウンターを減少（フレームレートに基づく）
    counter_ -= 1.0f / 60.0f;
    if (counter_ <= 0.0f) {
        counter_ = 0.0f;      // カウンターが負になるのを防ぐ
        fadeOutFinish = true; // フェードアウト完了フラグを立てる
        resetDone = false;    // 次回のために静的変数をリセット
    }
}

void SceneTransition::UpdateGrid() {
    for (auto &rect : gridRects_) {
        if (fadeInStart && !fadeInFinish) {
            // フェードイン：画面が黒く埋まる
            // 各矩形の開始時刻に達したかチェック
            if (counter_ >= rect.delayTime) {
                // まだフェード開始していなければ開始時刻を記録
                if (rect.fadeStartTime < 0.0f) {
                    rect.fadeStartTime = counter_;
                }

                // この矩形が実際にフェードしている時間
                float elapsedTime = counter_ - rect.fadeStartTime;
                float localProgress = elapsedTime / duration_;
                localProgress = min(localProgress, 1.0f);

                // フェードアウトと同じイージングを適用
                float eased = powf(localProgress, 0.3f);
                rect.currentAlpha = eased;
            } else {
                // まだ開始時刻に達していない
                rect.currentAlpha = 0.0f;
            }

        } else if (fadeOutStart && fadeInFinish && !fadeOutFinish) {
            // フェードアウト：次シーンへ移行（逆順で消える）
            // 最大遅延時間を取得
            float maxDelayTime = 0.0f;
            for (auto &r : gridRects_) {
                maxDelayTime = max(maxDelayTime, r.delayTime);
            }

            // 逆順の遅延時間を計算
            float reverseDelay = maxDelayTime - rect.delayTime;
            float totalDuration = maxDelayTime + duration_;

            // 現在の進行状況（カウンターが減少するので、開始時からの経過時間）
            float fadeOutElapsed = totalDuration - counter_;

            if (fadeOutElapsed >= reverseDelay) {
                // この矩形のフェード開始時刻を記録
                if (rect.fadeStartTime < 0.0f) {
                    rect.fadeStartTime = fadeOutElapsed;
                }

                // この矩形が実際にフェードしている時間
                float elapsedTime = fadeOutElapsed - rect.fadeStartTime;
                float localProgress = elapsedTime / duration_;
                localProgress = min(localProgress, 1.0f);

                // より急激に透明度を下げる
                float eased = powf(localProgress, 0.3f);
                rect.currentAlpha = 1.0f - eased;
            } else {
                // まだ開始時刻に達していない（完全に不透明のまま）
                rect.currentAlpha = 1.0f;
            }
        }

        // スプライトのアルファ値を更新
        rect.sprite->SetAlpha(rect.currentAlpha);
    }
}

// トランジション状態をリセット
void SceneTransition::Reset() {
    counter_ = 0.0f;
    fadeInFinish = false;
    fadeOutFinish = false;
    fadeInStart = false;
    fadeOutStart = false;
    isEnd = false;

    for (auto &rect : gridRects_) {
        rect.currentAlpha = 0.0f;
        rect.sprite->SetAlpha(0.0f);
        rect.fadeStartTime = -1.0f; // フェード開始時刻をリセット
    }
}