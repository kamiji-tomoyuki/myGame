#include "Frame.h"

std::chrono::high_resolution_clock::time_point Frame::lastTime_ = std::chrono::high_resolution_clock::now();
float Frame::deltaTime_ = 0.0f;

void Frame::Init() {
    lastTime_ = std::chrono::high_resolution_clock::now();
}

void Frame::Update() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed = currentTime - lastTime_;
    deltaTime_ = elapsed.count();
    lastTime_ = currentTime;
}

float Frame::DeltaTime() {
    return deltaTime_;
}
