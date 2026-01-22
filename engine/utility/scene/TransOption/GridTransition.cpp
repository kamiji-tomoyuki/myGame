#include "GridTransition.h"
#include "TextureManager.h"
#include <algorithm>
#include <cmath>

GridTransition::GridTransition() {}

GridTransition::~GridTransition() {}

void GridTransition::Initialize() {
    duration_ = 1.0f;
    counter_ = 0.0f;
    progress_ = 0.0f;
    transitionStart_ = false;
    isEnd_ = false;
    isCloseTransition_ = false;
    gridCols_ = 16;
    gridRows_ = 9;

    InitializeGridRects();
}

void GridTransition::InitializeGridRects() {
    gridRects_.clear();

    // 各矩形のサイズを計算
    float rectWidth = screenWidth_ / gridCols_;
    float rectHeight = screenHeight_ / gridRows_;

    // チェッカーボードパターンで波の順番を計算
    totalWaves_ = 0;

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

            // チェッカーボードパターンで波のインデックスを設定
            // (行 + 列) の値で段階的に表示
            rect.waveIndex = row + col;
            totalWaves_ = max(totalWaves_, rect.waveIndex);

            // 遅延時間を計算（各波が順番に表示される）
            rect.delayTime = static_cast<float>(rect.waveIndex) / static_cast<float>(totalWaves_);
            rect.currentAlpha = 0.0f;

            gridRects_.push_back(std::move(rect));
        }
    }
}

void GridTransition::Update() {
    if (!transitionStart_ || isEnd_) {
        return;
    }

    ProcessTransition();
    UpdateGridRects();
}

void GridTransition::Draw() {
    // 全ての矩形を描画
    for (auto &rect : gridRects_) {
        rect.sprite->Draw();
    }
}

void GridTransition::ProcessTransition() {
    // 経過時間を更新
    counter_ += 1.0f / 60.0f;
    if (counter_ >= duration_) {
        counter_ = duration_;
        isEnd_ = true;
    }

    // 全体の進行度を計算
    progress_ = counter_ / duration_;
}

void GridTransition::UpdateGridRects() {
    for (auto &rect : gridRects_) {
        // 各矩形の個別進行度を計算（遅延を考慮）
        float localProgress = 0.0f;

        if (isCloseTransition_) {
            // 閉じる：画面が黒く埋まる
            // 遅延後に進行開始
            if (progress_ >= rect.delayTime) {
                localProgress = (progress_ - rect.delayTime) / (1.0f - rect.delayTime);
                localProgress = min(localProgress, 1.0f);
            }

            // イージング適用（easeOutCubic）
            float eased = 1.0f - powf(1.0f - localProgress, 3.0f);
            rect.currentAlpha = eased;

        } else {
            // 開く：次シーンへ移行（逆順で消える）
            float reverseDelay = 1.0f - rect.delayTime;
            if (progress_ >= reverseDelay) {
                localProgress = (progress_ - reverseDelay) / (1.0f - reverseDelay);
                localProgress = min(localProgress, 1.0f);
            }

            // イージング適用（easeInCubic）
            float eased = powf(localProgress, 3.0f);
            rect.currentAlpha = 1.0f - eased;
        }

        // スプライトのアルファ値を更新
        rect.sprite->SetAlpha(rect.currentAlpha);
    }
}

void GridTransition::Reset() {
    counter_ = 0.0f;
    progress_ = 0.0f;
    transitionStart_ = false;
    isEnd_ = false;

    for (auto &rect : gridRects_) {
        rect.currentAlpha = 0.0f;
        rect.sprite->SetAlpha(0.0f);
    }
}