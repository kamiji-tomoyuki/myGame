#include "Audio.h"
#include <cassert>
#include <fstream>

Audio* Audio::instance = nullptr;

void Audio::Initialize(const std::string& directoryPath)
{
	HRESULT hr;

	directoryPath_ = directoryPath;

	hr = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	hr = xAudio2->CreateMasteringVoice(&masterVoice);

}

Audio* Audio::GetInstance()
{
	if (instance == nullptr) {
		instance = new Audio;
	}
	return instance;
}

uint32_t Audio::LoadWave(const std::string& filename) {
	// --- wavファイル読み込み ---
	if (loadedFiles.find(filename) != loadedFiles.end()) {
		for (size_t i = 0; i < kMaxSoundData; ++i) {
			if (soundDatas_[i].name_ == filename) {
				return static_cast<uint32_t>(i);
			}
		}
	}

	std::string fullPath = directoryPath_ + "/" + filename;

	std::ifstream file;
	file.open(fullPath, std::ios_base::binary);
	assert(file.is_open());

	RiffHeader riff;
	file.read((char*)&riff, sizeof(riff));

	if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
		assert(0);
	}

	// タイプがWAVEかチェック
	if (strncmp(riff.type, "WAVE", 4) != 0) {
		assert(0);
	}

	ChunkHeader chunkHeader;
	FormatChunk format = {};

	while (file.read((char*)&chunkHeader, sizeof(chunkHeader))) {
		if (strncmp(chunkHeader.id, "fmt ", 4) == 0) {
			assert(chunkHeader.size <= sizeof(format.fmt));

			format.chunk = chunkHeader;
			file.read((char*)&format.fmt, chunkHeader.size);

			break;
		} else {
			file.seekg(chunkHeader.size, std::ios_base::cur);
		}
	}

	if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
		assert(0);
	}

	// --- チャンク読み込みとスキップ処理 ---
	ChunkHeader data;
	while (file.read((char*)&data, sizeof(data))) {
		if (strncmp(data.id, "data", 4) == 0) {
			break;
		} else {
			file.seekg(data.size, std::ios_base::cur);
		}
	}

	if (strncmp(data.id, "data", 4) != 0) {
		assert(0);
	}

	std::vector<uint8_t> buffer(data.size);
	file.read(reinterpret_cast<char*>(buffer.data()), data.size);

	file.close();

	SoundData& soundData = soundDatas_[soundDataIndex];
	soundData.wfex = format.fmt;
	soundData.buffer = std::move(buffer); 
	soundData.name_ = filename;

	loadedFiles.insert(filename);

	uint32_t currentIndex = static_cast<uint32_t>(soundDataIndex);

	soundDataIndex = (soundDataIndex + 1) % kMaxSoundData;

	return currentIndex;
}



void Audio::Unload(uint32_t soundIndex) {
	SoundData& soundData = soundDatas_[soundIndex];

	soundData.buffer.clear();  // バッファを空にする
	soundData.wfex = {};
	soundData.name_.clear();
}

void Audio::PlayWave(uint32_t soundIndex, float volume, bool loop) {
	HRESULT result;

	// --- 再生 ---
	const SoundData& soundData = soundDatas_[soundIndex];

	Voice* voice = new Voice();
	voice->handle = soundIndex;
	voice->volume = volume;

	VoiceCallback* voiceCallback = new VoiceCallback();

	result = xAudio2->CreateSourceVoice(&voice->sourceVoice, &soundData.wfex, 0, XAUDIO2_DEFAULT_FREQ_RATIO, voiceCallback);
	assert(SUCCEEDED(result));

	// --- バッファを設定 ---
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.buffer.data();
	buf.AudioBytes = static_cast<uint32_t>(soundData.buffer.size());
	buf.Flags = XAUDIO2_END_OF_STREAM;
	buf.pContext = voice;

	// --- ループ再生の設定 ---
	if (loop) {
		buf.LoopCount = XAUDIO2_LOOP_INFINITE;  // 無限ループで再生
	}
	else {
		buf.LoopCount = 0;  // ループしない
	}

	result = voice->sourceVoice->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(result));

	result = voice->sourceVoice->Start();
	assert(SUCCEEDED(result));

	// --- 音量を設定 ---
	voice->sourceVoice->SetVolume(voice->volume);

	// 再生中のボイスをセットに追加
	voices_.insert(voice);
}

void Audio::StopWave(uint32_t soundIndex)
{
	// --- 音を停止 ---
	for (auto it = voices_.begin(); it != voices_.end(); ) {
		if ((*it)->handle == soundIndex) {
			if ((*it)->sourceVoice != nullptr) {
				(*it)->sourceVoice->Stop(0);
				(*it)->sourceVoice->DestroyVoice();
			}
			delete* it; 
			it = voices_.erase(it);
		}
		else {
			++it;
		}
	}
}

void Audio::SetVolume(uint32_t soundIndex, float volume)
{
	for (auto& voice : voices_) {
		if (voice->handle == soundIndex) {
			voice->volume = volume;
			voice->sourceVoice->SetVolume(volume);
			break;
		}
	}
}

void Audio::Finalize()
{
	if (masterVoice) {
		masterVoice->DestroyVoice();
		masterVoice = nullptr;
	}

	for (auto voice : voices_) {
		if (voice->sourceVoice) {
			voice->sourceVoice->DestroyVoice();

		}
		delete voice;
	}
	if (xAudio2) {
		xAudio2.Reset();
	}

	voices_.clear();
	delete instance;
	instance = nullptr;
}

