#include "pch.h"
#include "SimpleSynthStudio/ProjectExporter.h"

#include "Audio/AudioEngine.h"
#include "Audio/SynthLibrary.h"
#include "Audio/WavFile.h"
#include "SimpleSynthStudio/LibraryExporter.h"
#include "SimpleSynthStudio/SynthProject.h"

namespace SimpleSynthStudio
{
	namespace
	{
		std::vector<std::uint8_t> EncodeLayers(const std::vector<SynthLayer>& layers)
		{
			const std::vector<std::int16_t> samples = Synth::Mix(layers, AudioEngine::SAMPLE_RATE);
			return WavFile::Encode(samples, AudioEngine::SAMPLE_RATE);
		}
	}

	std::vector<ZipEntry> ProjectExporter::BuildEntries(const SynthDocument& document, const std::string& root)
	{
		std::vector<ZipEntry> entries;

		const std::string projectText = SynthProject::Serialize(document);
		const std::vector<std::uint8_t> projectBytes(projectText.begin(), projectText.end());
		entries.push_back({ std::format("{}/{}{}", root, root, SynthProject::EXTENSION), projectBytes });

		std::vector<std::string> used;
		for (const CustomEffect& effect : document.Library) {
			std::vector<SynthLayer> layers;
			for (const TimelineClip& clip : effect.Clips) {
				layers.push_back({ clip.Voice, clip.Start });
			}

			const std::string baseName = LibraryExporter::ToFileName(effect.Name);
			std::string candidate = baseName;
			for (int number = 2; std::find(used.begin(), used.end(), candidate) != used.end(); number++) {
				candidate = std::format("{}-{}", baseName, number);
			}
			used.push_back(candidate);
			entries.push_back({ std::format("{}/sounds/{}.wav", root, candidate), EncodeLayers(layers) });
		}

		for (const SynthEffect& effect : SynthLibrary::GetEffects()) {
			const std::string category = LibraryExporter::ToFileName(effect.Category);
			const std::string name = LibraryExporter::ToFileName(effect.Name);
			entries.push_back({ std::format("{}/templates/{}/{}.wav", root, category, name), EncodeLayers(effect.Layers) });
		}
		return entries;
	}
}
