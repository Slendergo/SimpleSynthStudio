#pragma once

#include "Audio/Synth.h"

namespace SimpleSynthStudio
{
	struct SynthEffect
	{
		std::string Name;
		std::string Category;
		std::vector<SynthLayer> Layers;

		float GetLength() const
		{
			float length = 0.0f;
			for (const SynthLayer& layer : Layers) {
				length = std::max(length, layer.Start + layer.Voice.Duration + layer.Voice.Release);
			}
			return length;
		}
	};

	class SynthLibrary
	{
	public:
		static const std::vector<SynthEffect>& GetEffects();
		static const std::vector<std::string>& GetCategories();
	};
}
