#pragma once

namespace SimpleSynthStudio
{
	class FileDialog
	{
	public:
		static bool IsAvailable();

		static std::optional<std::filesystem::path> Open(const std::string& title, const std::string& description, const std::string& extension, const std::filesystem::path& start);
		static std::optional<std::filesystem::path> Save(const std::string& title, const std::string& description, const std::string& extension, const std::filesystem::path& start);
		static std::optional<std::filesystem::path> Folder(const std::string& title, const std::filesystem::path& start);
	};
}
