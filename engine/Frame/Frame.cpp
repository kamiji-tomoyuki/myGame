#include "Frame.h"
#include <chrono>

/// <summary>
/// 静的メンバ変数の定義
/// </summary>
// 静的メンバ変数の定義
namespace Engine {
std::chrono::high_resolution_clock::time_point Frame::lastTime_ = std::chrono::high_resolution_clock::now();
std::chrono::high_resolution_clock::time_point Frame::fpsCalcTime_ = std::chrono::high_resolution_clock::now();
float Frame::deltaTime_ = 0.0f;
float Frame::fps_ = 0.0f;
int Frame::frameCount_ = 0;

/// <summary>
/// フレームの初期化処理
/// </summary>
void Frame::Init() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    lastTime_ = currentTime;
    fpsCalcTime_ = currentTime;
    deltaTime_ = 0.0f;
    fps_ = 0.0f;
    frameCount_ = 0;
}

/// <summary>
/// フレームの更新処理
/// </summary>
void Frame::Update() {
    // 現在の時刻を取得
    auto currentTime = std::chrono::high_resolution_clock::now();

    // 前回フレームからの経過時間を計算
    std::chrono::duration<float> elapsed = currentTime - lastTime_;
    deltaTime_ = elapsed.count(); // 秒単位の経過時間

    // フレームスパイク時に時間が飛びすぎるのを防ぐ（コンボ受付窓の飛ばし等を防止）。
    // 20fps相当を下限とし、それ以下では時間が引き延びる代わりに処理飛びを防ぐ。
    constexpr float kMaxDeltaTime = 0.05f;
    if (deltaTime_ > kMaxDeltaTime) { deltaTime_ = kMaxDeltaTime; }

    // フレームカウントを増加
    frameCount_++;

    // FPSを毎フレーム更新（1秒ごとに平均を計算）
    std::chrono::duration<float> fpsElapsed = currentTime - fpsCalcTime_;
    if (fpsElapsed.count() >= 0.5f) { // 0.5秒ごとに更新（より安定した値を得るため）
        // 平均FPSを計算
        fps_ = static_cast<float>(frameCount_) / fpsElapsed.count();

        // カウンタをリセット
        frameCount_ = 0;
        fpsCalcTime_ = currentTime;
    }

    // 次回の更新のために現在の時刻を記録
    lastTime_ = currentTime;
}

/// <summary>
/// 前回の更新からの経過時間を取得
/// </summary>
/// <returns>前回の更新からの経過時間</returns>
float Frame::DeltaTime() {
    return deltaTime_;
}

/// <summary>
/// 現在のFPSを取得
/// </summary>
/// <returns>現在のFPS</returns>
float Frame::GetFPS() {
    return fps_; // 現在のFPSを返す
}
} // namespace Engine
