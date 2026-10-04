#pragma once

namespace SimpleSynthStudio
{
	struct ZipEntry
	{
		std::string Name;
		std::vector<std::uint8_t> Data;
	};

	class ZipFile
	{
	public:
		static bool Write(const std::filesystem::path& path, const std::vector<ZipEntry>& entries);
	};
}
