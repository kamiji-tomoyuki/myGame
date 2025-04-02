#pragma once
#include "xaudio2.h"
#include "array"
#include "cstdint"
#include "string"
#include "vector"
#include "wrl.h"
#include <set>

// 音声管理
class Audio
{
	class VoiceCallback : public IXAudio2VoiceCallback {
	
	public:
		void STDMETHODCALLTYPE OnStreamEnd() override {}
		void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
		void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
		void STDMETHODCALLTYPE OnBufferStart(void*) override {}
		void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
		void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override {}

		void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override {
			if (pBufferContext) {
				Voice* voice = reinterpret_cast<Voice*>(pBufferContext);
				if (voice) {
					Audio::GetInstance()->StopWave(voice->handle);
				}
			}
		}
	};

private:

	static const int kMaxSoundData = 2108;

	// --- 構造体 ---
	struct ChunkHeader
	{
		char id[4];
		int32_t size;
	};

	struct RiffHeader
	{
		ChunkHeader chunk;
		char type[4];
	};

	struct FormatChunk
	{
		ChunkHeader chunk;
		WAVEFORMATEX fmt;
	};

	struct SoundData {
		WAVEFORMATEX wfex;
		std::vector<uint8_t> buffer;
		std::string name_;
	};

	struct Voice {
		uint32_t handle = 0u;
		IXAudio2SourceVoice* sourceVoice = nullptr;
		float volume = 1.0f;
	};

#pragma region シングルトンインスタンス
private:
	static Audio* instance;

	Audio() = default;
	~Audio() = default;
	Audio(Audio&) = delete;
	Audio& operator = (Audio&) = delete;

public:
	// シングルトンインスタンスの取得
	static Audio* GetInstance();
	// 終了
	void Finalize();
#pragma endregion シングルトンインスタンス

public:

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="directoryPath"></param>
	void Initialize(const std::string& directoryPath = "resources/sounds");

	/// <summary>
	/// 音声読み込み
	/// </summary>
	/// <param name="filename"></param>
	/// <returns></returns>
	uint32_t LoadWave(const std::string& filename);

	/// <summary>
	/// 音声データ解放
	/// </summary>
	/// <param name="soundData"></param>
	void Unload(uint32_t soundIndex);

	/// <summary>
	/// 音声再生
	/// </summary>
	/// <param name="xAudio2"></param>
	/// <param name="soundData"></param>
	void PlayWave(uint32_t soundIndex, float volume, bool loop = false);

	/// <summary>
	/// 音声停止
	/// </summary>
	/// <param name="soundIndex"></param>
	void StopWave(uint32_t soundIndex);

	/// <summary>
	/// 音量設定
	/// </summary>
	/// <param name="soundIndex"></param>
	/// <param name="volume"></param>
	void SetVolume(uint32_t soundIndex, float volume);

private:

	Microsoft::WRL::ComPtr<IXAudio2>xAudio2;

	IXAudio2MasteringVoice* masterVoice;
	std::string directoryPath_;
	std::array<SoundData, kMaxSoundData> soundDatas_;
	size_t soundDataIndex = 0;
	std::set<Voice*> voices_;
	std::set<std::string> loadedFiles;

	// フォーマット情報を読み込む
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
};
