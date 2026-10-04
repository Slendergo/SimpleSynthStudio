#include "pch.h"
#include "Audio/Synth.h"

namespace SimpleSynthStudio
{
	namespace
	{
		constexpr float TWO_PI = std::numbers::pi_v<float> * 2.0f;

		float NextNoise(std::uint32_t& noiseState)
		{
			noiseState ^= noiseState << 13;
			noiseState ^= noiseState >> 17;
			noiseState ^= noiseState << 5;
			return static_cast<float>(noiseState) / 2147483648.0f - 1.0f;
		}

		float SampleWave(const SynthVoice& voice, float phase, bool wrapped, std::uint32_t& noiseState, float& held)
		{
			switch (voice.Wave) {
			case Waveform::Sine:
				return std::sin(phase * TWO_PI);
			case Waveform::Square:
				return phase < voice.PulseWidth ? 1.0f : -1.0f;
			case Waveform::Saw:
				return phase * 2.0f - 1.0f;
			case Waveform::Triangle:
				return 4.0f * std::abs(phase - 0.5f) - 1.0f;
			case Waveform::Noise:
				return NextNoise(noiseState);
			case Waveform::Crunch:
				if (wrapped) {
					held = NextNoise(noiseState);
				}
				return held;
			}
			return 0.0f;
		}

		float HeldLevel(const SynthVoice& voice, float time)
		{
			if (time < voice.Attack) {
				return voice.Attack > 0.0f ? time / voice.Attack : 1.0f;
			}

			const float decayTime = time - voice.Attack;
			if (decayTime < voice.Decay) {
				return std::lerp(1.0f, voice.Sustain, decayTime / voice.Decay);
			}
			return voice.Sustain;
		}
	}

	SynthVoice Synth::GetPreset(SynthPreset preset)
	{
		SynthVoice voice;
		switch (preset) {
		case SynthPreset::Laser:
			voice.Wave = Waveform::Square;
			voice.Frequency = 1400.0f;
			voice.EndFrequency = 200.0f;
			voice.Duration = 0.18f;
			voice.Attack = 0.001f;
			voice.Decay = 0.05f;
			voice.Sustain = 0.5f;
			voice.Release = 0.05f;
			voice.Volume = 0.35f;
			break;
		case SynthPreset::Hit:
			voice.Wave = Waveform::Noise;
			voice.Duration = 0.05f;
			voice.Attack = 0.001f;
			voice.Decay = 0.04f;
			voice.Sustain = 0.2f;
			voice.Release = 0.08f;
			voice.Volume = 0.5f;
			break;
		case SynthPreset::Pickup:
			voice.Wave = Waveform::Triangle;
			voice.Frequency = 660.0f;
			voice.EndFrequency = 1320.0f;
			voice.Duration = 0.12f;
			voice.Attack = 0.005f;
			voice.Decay = 0.03f;
			voice.Sustain = 0.8f;
			voice.Release = 0.08f;
			voice.Volume = 0.5f;
			break;
		case SynthPreset::Explosion:
			voice.Wave = Waveform::Noise;
			voice.Duration = 0.25f;
			voice.Attack = 0.002f;
			voice.Decay = 0.3f;
			voice.Sustain = 0.3f;
			voice.Release = 0.5f;
			voice.Volume = 0.7f;
			break;
		case SynthPreset::Blip:
			voice.Wave = Waveform::Sine;
			voice.Frequency = 880.0f;
			voice.EndFrequency = 880.0f;
			voice.Duration = 0.08f;
			voice.Attack = 0.003f;
			voice.Decay = 0.02f;
			voice.Sustain = 0.8f;
			voice.Release = 0.05f;
			voice.Volume = 0.5f;
			break;
		}
		return voice;
	}

	std::vector<std::int16_t> Synth::Render(const SynthVoice& voice, std::uint32_t sampleRate)
	{
		const float totalTime = voice.Duration + voice.Release;
		const std::size_t sampleCount = static_cast<std::size_t>(totalTime * static_cast<float>(sampleRate));
		const float releaseStartLevel = HeldLevel(voice, voice.Duration);
		const float dt = 1.0f / static_cast<float>(sampleRate);

		std::vector<std::int16_t> samples;
		samples.reserve(sampleCount);

		float phase = 0.0f;
		float held = 0.0f;
		float filtered = 0.0f;
		std::uint32_t noiseState = 0x9E3779B9u;
		for (std::size_t i = 0; i < sampleCount; i++) {
			const float time = static_cast<float>(i) * dt;
			const float progress = totalTime > 0.0f ? time / totalTime : 0.0f;

			const float vibrato = std::sin(time * voice.VibratoRate * TWO_PI) * voice.VibratoDepth;
			const bool exponential = voice.Frequency > 0.0f && voice.EndFrequency > 0.0f;
			const float slide = exponential ? voice.Frequency * std::pow(voice.EndFrequency / voice.Frequency, progress) : std::lerp(voice.Frequency, voice.EndFrequency, progress);
			const float frequency = slide * (1.0f + vibrato);
			phase += frequency * dt;
			const bool wrapped = phase >= 1.0f;
			phase -= std::floor(phase);

			float level = releaseStartLevel;
			if (time < voice.Duration) {
				level = HeldLevel(voice, time);
			} else if (voice.Release > 0.0f) {
				level = releaseStartLevel * (1.0f - (time - voice.Duration) / voice.Release);
			}

			float sample = SampleWave(voice, phase, wrapped, noiseState, held);
			if (voice.Cutoff > 0.0f) {
				const float endCutoff = voice.EndCutoff > 0.0f ? voice.EndCutoff : voice.Cutoff;
				const float cutoff = voice.Cutoff * std::pow(endCutoff / voice.Cutoff, progress);
				const float alpha = 1.0f - std::exp(-TWO_PI * cutoff * dt);
				filtered += alpha * (sample - filtered);
				sample = filtered;
			}

			const float value = sample * level * voice.Volume;
			const float clamped = std::clamp(value, -1.0f, 1.0f);
			samples.push_back(static_cast<std::int16_t>(clamped * 32767.0f));
		}
		return samples;
	}

	std::vector<std::int16_t> Synth::Mix(const std::vector<SynthLayer>& layers, std::uint32_t sampleRate)
	{
		const float rate = static_cast<float>(sampleRate);

		float length = 0.0f;
		for (const SynthLayer& layer : layers) {
			length = std::max(length, layer.Start + layer.Voice.Duration + layer.Voice.Release);
		}

		const std::size_t total = static_cast<std::size_t>(length * rate) + 1;
		std::vector<float> mix(total, 0.0f);
		for (const SynthLayer& layer : layers) {
			const std::vector<std::int16_t> samples = Render(layer.Voice, sampleRate);
			const std::size_t offset = static_cast<std::size_t>(layer.Start * rate);
			for (std::size_t i = 0; i < samples.size() && offset + i < total; i++) {
				mix[offset + i] += static_cast<float>(samples[i]) / 32768.0f;
			}
		}

		float peak = 1.0f;
		for (const float value : mix) {
			peak = std::max(peak, std::abs(value));
		}

		std::vector<std::int16_t> output;
		output.reserve(total);
		for (const float value : mix) {
			output.push_back(static_cast<std::int16_t>(value / peak * 32767.0f));
		}
		return output;
	}
}
