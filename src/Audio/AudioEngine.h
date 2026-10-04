#pragma once

#include "Audio/AudioBuffer.h"
#include "Audio/Synth.h"

namespace SimpleSynthStudio
{
	struct Vec3
	{
		float X = 0.0f;
		float Y = 0.0f;
		float Z = 0.0f;
	};

	struct AudioPlayProperties
	{
		Vec3 Position;
		float Volume = 1.0f;
		float Pitch = 1.0f;
		float ReferenceDistance = 4.0f;
		float MaxDistance = 40.0f;
		bool Spatial = true;
	};

	class AudioEngine
	{
	public:
		static constexpr std::uint32_t MAX_SOURCES = 32;
		static constexpr std::uint32_t SAMPLE_RATE = 44100;

		AudioEngine();
		~AudioEngine();

		AudioEngine(const AudioEngine&) = delete;
		AudioEngine& operator=(const AudioEngine&) = delete;

		float GetMasterVolume() const { return _masterVolume; }
		
		bool IsInitialized() const { return _device != nullptr; }

		bool Initialize();
		void Shutdown();

		void SetMasterVolume(float volume);
		void SetListener(const Vec3& position, const Vec3& forward, const Vec3& up, const Vec3& velocity);

		std::shared_ptr<AudioBuffer> CreateBuffer(const std::vector<std::int16_t>& samples, std::uint32_t sampleRate);
		std::shared_ptr<AudioBuffer> CreateSynthBuffer(const SynthVoice& voice);
		std::shared_ptr<AudioBuffer> CreateSynthBuffer(SynthPreset preset);

		bool Play(const std::shared_ptr<AudioBuffer>& buffer, const AudioPlayProperties& properties);
		void StopAll();
		void Update();

	private:
		void* _device = nullptr;
		void* _context = nullptr;

		float _masterVolume = 1.0f;

		std::array<std::uint32_t, MAX_SOURCES> _sources = {};
		std::array<std::shared_ptr<AudioBuffer>, MAX_SOURCES> _playing;
	};
}
