#pragma once

namespace SimpleSynthStudio
{
	enum class Waveform : std::uint8_t
	{
		Sine,
		Square,
		Saw,
		Triangle,
		Noise,
		Crunch
	};

	enum class SynthPreset : std::uint8_t
	{
		Laser,
		Hit,
		Pickup,
		Explosion,
		Blip
	};

	struct SynthVoice
	{
		Waveform Wave = Waveform::Sine;
		float Frequency = 440.0f;
		float EndFrequency = 440.0f;
		float Duration = 0.3f;
		float Attack = 0.01f;
		float Decay = 0.05f;
		float Sustain = 0.7f;
		float Release = 0.1f;
		float VibratoRate = 0.0f;
		float VibratoDepth = 0.0f;
		float Volume = 0.6f;
		float Cutoff = 0.0f;
		float EndCutoff = 0.0f;
		float PulseWidth = 0.5f;
	};

	struct SynthLayer
	{
		SynthVoice Voice;
		float Start = 0.0f;
	};

	class Synth
	{
	public:
		static std::vector<std::int16_t> Mix(const std::vector<SynthLayer>& layers, std::uint32_t sampleRate);
		static std::vector<std::int16_t> Render(const SynthVoice& voice, std::uint32_t sampleRate);
		static SynthVoice GetPreset(SynthPreset preset);
	};
}
