#pragma once

#include "SimpleSynthStudio/SynthDocument.h"

namespace SimpleSynthStudio
{
	class SynthProject
	{
	public:
		static constexpr const char* EXTENSION = ".psynth";
		static constexpr int VERSION = 1;

		static std::string Serialize(const SynthDocument& document);
		static bool Parse(const std::string& text, SynthDocument& document);

		static bool Save(const std::filesystem::path& path, const SynthDocument& document);
		static bool Load(const std::filesystem::path& path, SynthDocument& document);
	};
}
