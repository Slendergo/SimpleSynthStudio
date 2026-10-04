#include "pch.h"
#include "SimpleSynthStudio/LibraryExporter.h"

namespace SimpleSynthStudio
{
	std::string LibraryExporter::ToFileName(const std::string& name)
	{
		std::string result;
		result.reserve(name.size());
		for (const char c : name) {
			const unsigned char value = static_cast<unsigned char>(c);
			if (std::isalnum(value) || c == '_') {
				result.push_back(static_cast<char>(std::tolower(value)));
			} else if (result.empty() || result.back() != '-') {
				result.push_back('-');
			}
		}

		while (!result.empty() && result.back() == '-') {
			result.pop_back();
		}
		return result.empty() ? std::string("sound") : result;
	}
}
