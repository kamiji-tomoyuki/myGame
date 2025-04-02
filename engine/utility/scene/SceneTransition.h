#pragma once
#include"Sprite.h"
#include"memory"
class SceneTransition
{
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
	/// <param name="start"></param>
	void SetFadeInStart(bool start) { fadeInStart = start; }
	void SetFadeOutStart(bool start) { fadeOutStart = start; }
	void SetFadeInFinish(bool finish) { fadeInFinish = finish; }

	/// <summary>
	/// getter
	/// </summary>
	/// <returns></returns>
	bool IsEnd() { return isEnd; }
	bool FadeInFinish() { return fadeInFinish; }
	bool FadeInStart() { return fadeInStart; }

	/// <summary>
	/// リセット
	/// </summary>
	void Reset();

private:

	/// <summary>
	/// フェードイン
	/// </summary>
	void FadeIn();

	/// <summary>
	/// フェードアウト
	/// </summary>
	void FadeOut();

private:
	// フェードの持続時間
	float duration_ = 0.0f;
	// 経過時間カウンター
	float counter_ = 0.0f;

	std::unique_ptr<Sprite> sprite_ = nullptr;

	uint32_t texture = 0u;

	bool fadeInStart = false;
	bool fadeOutStart = false;
	bool fadeInFinish = false;
	bool fadeOutFinish = false;
	bool isEnd = false;
	float In_t = 0.0f;
	float Out_t = 0.0f;

};

