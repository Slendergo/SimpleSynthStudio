#pragma once

#include "Audio/ZipFile.h"
#include "SimpleSynthStudio/SynthDocument.h"

namespace SimpleSynthStudio
{
	class ProjectExporter
	{
	public:
		static std::vector<ZipEntry> BuildEntries(const SynthDocument& document, const std::string& root);
	};
}
