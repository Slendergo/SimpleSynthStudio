#include "pch.h"
#include "SimpleSynthStudio/SynthProject.h"

namespace SimpleSynthStudio
{
	namespace
	{
		constexpr std::array<const char*, 6> WAVEFORM_NAMES = { "sine", "square", "saw", "triangle", "noise", "crunch" };

		std::optional<Waveform> ParseWaveform(const std::string& name)
		{
			for (std::size_t i = 0; i < WAVEFORM_NAMES.size(); i++) {
				if (name == WAVEFORM_NAMES[i]) {
					return static_cast<Waveform>(i);
				}
			}
			return std::nullopt;
		}

		bool ParseFloats(std::istringstream& stream, std::vector<float>& values)
		{
			values.clear();
			std::string token;
			while (stream >> token) {
				char* end = nullptr;
				const float value = std::strtof(token.c_str(), &end);
				if (end != token.c_str() + token.size() || !std::isfinite(value)) {
					return false;
				}
				values.push_back(value);
			}
			return true;
		}

		void AppendClip(std::string& text, const TimelineClip& clip)
		{
			const SynthVoice& voice = clip.Voice;
			text += "clip\n";
			text += std::format("wave {}\n", WAVEFORM_NAMES[static_cast<std::size_t>(voice.Wave)]);
			text += std::format("start {}\n", clip.Start);
			text += std::format("position {} {}\n", clip.X, clip.Z);
			text += std::format("frequency {} {}\n", voice.Frequency, voice.EndFrequency);
			text += std::format("duration {}\n", voice.Duration);
			text += std::format("attack {}\n", voice.Attack);
			text += std::format("decay {}\n", voice.Decay);
			text += std::format("sustain {}\n", voice.Sustain);
			text += std::format("release {}\n", voice.Release);
			text += std::format("vibrato {} {}\n", voice.VibratoRate, voice.VibratoDepth);
			text += std::format("volume {}\n", voice.Volume);
			text += std::format("cutoff {} {}\n", voice.Cutoff, voice.EndCutoff);
			text += std::format("pulse {}\n", voice.PulseWidth);
			text += "end\n";
		}

		bool ApplyKey(TimelineClip& clip, const std::string& key, const std::vector<float>& values)
		{
			SynthVoice& voice = clip.Voice;
			if (key == "start" && values.size() == 1) {
				clip.Start = values[0];
			} else if (key == "position" && values.size() == 2) {
				clip.X = values[0];
				clip.Z = values[1];
			} else if (key == "frequency" && values.size() == 2) {
				voice.Frequency = values[0];
				voice.EndFrequency = values[1];
			} else if (key == "duration" && values.size() == 1) {
				voice.Duration = values[0];
			} else if (key == "attack" && values.size() == 1) {
				voice.Attack = values[0];
			} else if (key == "decay" && values.size() == 1) {
				voice.Decay = values[0];
			} else if (key == "sustain" && values.size() == 1) {
				voice.Sustain = values[0];
			} else if (key == "release" && values.size() == 1) {
				voice.Release = values[0];
			} else if (key == "vibrato" && values.size() == 2) {
				voice.VibratoRate = values[0];
				voice.VibratoDepth = values[1];
			} else if (key == "volume" && values.size() == 1) {
				voice.Volume = values[0];
			} else if (key == "cutoff" && values.size() == 2) {
				voice.Cutoff = values[0];
				voice.EndCutoff = values[1];
			} else if (key == "pulse" && values.size() == 1) {
				voice.PulseWidth = values[0];
			} else {
				return false;
			}
			return true;
		}
	}

	std::string SynthProject::Serialize(const SynthDocument& document)
	{
		std::string text = std::format("psynth {}\nrepeat {}\nactive {}\n", VERSION, document.Repeat ? 1 : 0, document.Active);
		for (const TimelineClip& clip : document.Clips) {
			AppendClip(text, clip);
		}

		for (const CustomEffect& effect : document.Library) {
			std::string name = effect.Name;
			std::replace(name.begin(), name.end(), '\n', ' ');
			std::replace(name.begin(), name.end(), '\r', ' ');
			text += std::format("effect {}\n", name);
			for (const TimelineClip& clip : effect.Clips) {
				AppendClip(text, clip);
			}
			text += "endeffect\n";
		}
		return text;
	}

	bool SynthProject::Parse(const std::string& text, SynthDocument& document)
	{
		std::istringstream input(text);
		std::string line;

		if (!std::getline(input, line)) {
			return false;
		}

		std::istringstream header(line);
		std::string magic;
		int version = 0;
		if (!(header >> magic >> version) || magic != "psynth" || version < 1 || version > VERSION) {
			return false;
		}

		SynthDocument result;
		TimelineClip clip;
		bool inClip = false;
		std::optional<CustomEffect> effect;
		while (std::getline(input, line)) {
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}

			std::istringstream stream(line);
			std::string key;
			if (!(stream >> key)) {
				continue;
			}

			if (key == "effect") {
				std::string name;
				std::getline(stream, name);
				const std::size_t first = name.find_first_not_of(' ');
				effect = CustomEffect();
				effect->Name = first == std::string::npos ? std::string("Effect") : name.substr(first);
			} else if (key == "endeffect") {
				if (effect) {
					result.Library.push_back(std::move(*effect));
				}
				effect.reset();
			} else if (key == "clip") {
				clip = TimelineClip();
				inClip = true;
			} else if (key == "end") {
				if (inClip && effect) {
					effect->Clips.push_back(clip);
				} else if (inClip) {
					result.Clips.push_back(clip);
				}
				inClip = false;
			} else if (key == "repeat") {
				int repeat = 0;
				stream >> repeat;
				result.Repeat = repeat != 0;
			} else if (key == "active") {
				stream >> result.Active;
			} else if (inClip && key == "wave") {
				std::string name;
				stream >> name;
				const std::optional<Waveform> wave = ParseWaveform(name);
				if (!wave) {
					return false;
				}
				clip.Voice.Wave = *wave;
			} else if (inClip) {
				std::vector<float> values;
				if (!ParseFloats(stream, values)) {
					return false;
				}
				ApplyKey(clip, key, values);
			}
		}

		result.Selected = result.Clips.empty() ? -1 : 0;
		if (result.Active < 0 || result.Active >= static_cast<int>(result.Library.size())) {
			result.Active = -1;
		}
		document = std::move(result);
		return true;
	}

	bool SynthProject::Save(const std::filesystem::path& path, const SynthDocument& document)
	{
		std::ofstream stream(path, std::ios::binary);
		if (!stream) {
			LOG_ERROR("Failed to open {} for writing.", path.string());
			return false;
		}

		const std::string text = Serialize(document);
		stream.write(text.data(), static_cast<std::streamsize>(text.size()));
		return stream.good();
	}

	bool SynthProject::Load(const std::filesystem::path& path, SynthDocument& document)
	{
		std::ifstream stream(path, std::ios::binary);
		if (!stream) {
			LOG_ERROR("Failed to open {}.", path.string());
			return false;
		}

		std::stringstream buffer;
		buffer << stream.rdbuf();
		if (!Parse(buffer.str(), document)) {
			LOG_ERROR("{} is not a valid psynth file.", path.string());
			return false;
		}
		return true;
	}
}
