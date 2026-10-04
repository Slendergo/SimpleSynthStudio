#include "pch.h"
#include "Audio/AudioEngine.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

namespace SimpleSynthStudio
{
	AudioEngine::AudioEngine()
	{
	}

	AudioEngine::~AudioEngine()
	{
		Shutdown();
	}

	bool AudioEngine::Initialize()
	{
		ALCdevice* device = alcOpenDevice(nullptr);
		if (!device) {
			LOG_ERROR("Failed to open the default audio device.");
			return false;
		}

		const ALCint attributes[] = { ALC_HRTF_SOFT, ALC_TRUE, 0 };
		ALCcontext* context = alcCreateContext(device, attributes);
		if (!context || !alcMakeContextCurrent(context)) {
			LOG_ERROR("Failed to create the audio context.");
			if (context) {
				alcDestroyContext(context);
			}
			alcCloseDevice(device);
			return false;
		}

		_device = device;
		_context = context;

		alGenSources(static_cast<ALsizei>(MAX_SOURCES), _sources.data());
		alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
		alListenerf(AL_GAIN, _masterVolume);

		const ALCchar* deviceName = alcGetString(device, ALC_ALL_DEVICES_SPECIFIER);
		LOG_INFO("Using {}.", deviceName ? deviceName : "the default device");
		return true;
	}

	void AudioEngine::Shutdown()
	{
		if (!_device) {
			return;
		}

		alSourceStopv(static_cast<ALsizei>(MAX_SOURCES), _sources.data());
		for (const std::uint32_t source : _sources) {
			alSourcei(source, AL_BUFFER, 0);
		}
		alDeleteSources(static_cast<ALsizei>(MAX_SOURCES), _sources.data());
		_sources = {};

		for (auto& buffer : _playing) {
			buffer.reset();
		}

		alcMakeContextCurrent(nullptr);
		alcDestroyContext(static_cast<ALCcontext*>(_context));
		alcCloseDevice(static_cast<ALCdevice*>(_device));
		_context = nullptr;
		_device = nullptr;
	}

	void AudioEngine::SetMasterVolume(float volume)
	{
		_masterVolume = std::clamp(volume, 0.0f, 1.0f);
		if (!_device) {
			return;
		}
		alListenerf(AL_GAIN, _masterVolume);
	}

	void AudioEngine::SetListener(const Vec3& position, const Vec3& forward, const Vec3& up, const Vec3& velocity)
	{
		if (!_device) {
			return;
		}

		const ALfloat orientation[6] = { forward.X, forward.Y, forward.Z, up.X, up.Y, up.Z };
		alListener3f(AL_POSITION, position.X, position.Y, position.Z);
		alListener3f(AL_VELOCITY, velocity.X, velocity.Y, velocity.Z);
		alListenerfv(AL_ORIENTATION, orientation);
	}

	std::shared_ptr<AudioBuffer> AudioEngine::CreateBuffer(const std::vector<std::int16_t>& samples, std::uint32_t sampleRate)
	{
		if (!_device || samples.empty()) {
			return nullptr;
		}

		ALuint handle = 0;
		alGenBuffers(1, &handle);

		const ALsizei byteCount = static_cast<ALsizei>(samples.size() * sizeof(std::int16_t));
		alBufferData(handle, AL_FORMAT_MONO16, samples.data(), byteCount, static_cast<ALsizei>(sampleRate));
		if (alGetError() != AL_NO_ERROR) {
			LOG_ERROR("Failed to upload a sound buffer.");
			alDeleteBuffers(1, &handle);
			return nullptr;
		}

		const float duration = static_cast<float>(samples.size()) / static_cast<float>(sampleRate);
		return std::make_shared<AudioBuffer>(handle, duration);
	}

	std::shared_ptr<AudioBuffer> AudioEngine::CreateSynthBuffer(const SynthVoice& voice)
	{
		const std::vector<std::int16_t> samples = Synth::Render(voice, SAMPLE_RATE);
		return CreateBuffer(samples, SAMPLE_RATE);
	}

	std::shared_ptr<AudioBuffer> AudioEngine::CreateSynthBuffer(SynthPreset preset)
	{
		const SynthVoice voice = Synth::GetPreset(preset);
		return CreateSynthBuffer(voice);
	}

	bool AudioEngine::Play(const std::shared_ptr<AudioBuffer>& buffer, const AudioPlayProperties& properties)
	{
		if (!_device || !buffer) {
			return false;
		}

		for (std::size_t i = 0; i < MAX_SOURCES; i++) {
			if (_playing[i]) {
				continue;
			}

			const ALuint source = _sources[i];
			const Vec3 position = properties.Spatial ? properties.Position : Vec3{};

			alSourcei(source, AL_BUFFER, static_cast<ALint>(buffer->GetHandle()));
			alSourcef(source, AL_GAIN, properties.Volume);
			alSourcef(source, AL_PITCH, properties.Pitch);
			alSourcei(source, AL_LOOPING, AL_FALSE);
			alSourcef(source, AL_REFERENCE_DISTANCE, properties.ReferenceDistance);
			alSourcef(source, AL_MAX_DISTANCE, properties.MaxDistance);
			alSourcef(source, AL_ROLLOFF_FACTOR, properties.Spatial ? 1.0f : 0.0f);
			alSourcei(source, AL_SOURCE_RELATIVE, properties.Spatial ? AL_FALSE : AL_TRUE);
			alSource3f(source, AL_POSITION, position.X, position.Y, position.Z);
			alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
			alSourcePlay(source);

			_playing[i] = buffer;
			return true;
		}
		return false;
	}

	void AudioEngine::StopAll()
	{
		if (!_device) {
			return;
		}

		alSourceStopv(static_cast<ALsizei>(MAX_SOURCES), _sources.data());
	}

	void AudioEngine::Update()
	{
		if (!_device) {
			return;
		}

		for (std::size_t i = 0; i < MAX_SOURCES; i++) {
			if (!_playing[i]) {
				continue;
			}

			ALint state = 0;
			alGetSourcei(_sources[i], AL_SOURCE_STATE, &state);
			if (state == AL_PLAYING) {
				continue;
			}

			alSourcei(_sources[i], AL_BUFFER, 0);
			_playing[i].reset();
		}
	}
}
