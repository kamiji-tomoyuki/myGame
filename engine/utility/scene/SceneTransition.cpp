#include "SceneTransition.h"
#include "TextureManager.h"
#include "algorithm"

SceneTransition::SceneTransition() {}

SceneTransition::~SceneTransition() {}

void SceneTransition::Initialize() {
	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize("white1x1.png", { 0, 0 });
	sprite_->SetSize(Vector2(1280, 720)); // 画面全体を覆うサイズ
	sprite_->SetColor(Vector3(0.0f, 0.0f, 0.0f)); // 黒い色
	sprite_->SetAlpha(0.0f); // 最初は完全に透明
	duration_ = 1.0f; // フェードの持続時間（例: 1秒）
	counter_ = 0.0f; // 経過時間カウンターを初期化
	fadeInFinish = false;
	fadeOutFinish = false;
	fadeInStart = false;
	fadeOutStart = false;
	isEnd = false;
}

void SceneTransition::Update() {
	if (fadeInStart) {
		// フェードイン中
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

	// トランジションが終了したら、終了フラグを立てる
	if (fadeInFinish && fadeOutFinish) {
		isEnd = true;
		fadeInStart = false;
		fadeOutStart = false;
	}

}

void SceneTransition::Draw() {
	sprite_->Draw();
}

void SceneTransition::FadeIn() {
	counter_ += 1.0f / 60.0f; // フレームレートを基にカウント（1フレームごとに0.0167秒進む）
	if (counter_ >= duration_) {
		counter_ = duration_; // 終了時間を超えないように制限
		fadeInFinish = true; // フェードイン終了フラグを立てる
	}

	// アルファ値の計算（0.0fから1.0fに増加）
	float alpha = counter_ / duration_;
	sprite_->SetAlpha(alpha);
}

void SceneTransition::FadeOut() {
	// カウンターを減少（フレームレートに基づく）
	counter_ -= 1.0f / 60.0f;
	if (counter_ <= 0.0f) {
		counter_ = 0.0f; // カウンターが負になるのを防ぐ
		fadeOutFinish = true; // フェードアウト完了フラグを立てる
	}

	// アルファ値の計算（1.0fから0.0fに減少）
	float alpha = counter_ / duration_; // カウンターが減るほどアルファも減る
	sprite_->SetAlpha(alpha); // アルファ値を設定
}


// トランジション状態をリセット
void SceneTransition::Reset() {
	counter_ = 0.0f;
	fadeInFinish = false;
	fadeOutFinish = false;
	fadeInStart = false;
	fadeOutStart = false;
	isEnd = false;
	sprite_->SetAlpha(0.0f); // 最初の透明状態に戻す
}
