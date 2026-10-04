#include "pch.h"
#include "SimpleSynthStudio/FileDialog.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commdlg.h>
#include <shobjidl.h>
#else
#include <cstdio>
#endif

namespace SimpleSynthStudio
{
	namespace
	{
		std::filesystem::path WithExtension(std::filesystem::path path, const std::string& extension)
		{
			if (path.extension() != extension) {
				path += extension;
			}
			return path;
		}
	}

#ifdef _WIN32
	namespace
	{
		std::wstring Widen(const std::string& text)
		{
			return std::wstring(text.begin(), text.end());
		}

		std::optional<std::filesystem::path> ShowFileDialog(bool save, const std::string& title, const std::string& description, const std::string& extension, const std::filesystem::path& start)
		{
			std::wstring filter = Widen(std::format("{} (*{})", description, extension));
			filter.push_back(L'\0');
			filter += Widen("*" + extension);
			filter.push_back(L'\0');
			filter += L"All files";
			filter.push_back(L'\0');
			filter += L"*.*";
			filter.push_back(L'\0');

			std::vector<wchar_t> buffer(32768, L'\0');
			const std::wstring name = start.filename().wstring();
			std::copy_n(name.begin(), std::min(name.size(), buffer.size() - 1), buffer.begin());

			std::error_code error;
			const std::wstring directory = std::filesystem::is_directory(start.parent_path(), error) ? start.parent_path().wstring() : std::wstring();
			const std::wstring windowTitle = Widen(title);
			const std::wstring defaultExtension = Widen(extension.empty() ? extension : extension.substr(1));

			OPENFILENAMEW dialog = {};
			dialog.lStructSize = sizeof(dialog);
			dialog.hwndOwner = GetActiveWindow();
			dialog.lpstrFilter = filter.c_str();
			dialog.lpstrFile = buffer.data();
			dialog.nMaxFile = static_cast<DWORD>(buffer.size());
			dialog.lpstrInitialDir = directory.empty() ? nullptr : directory.c_str();
			dialog.lpstrTitle = windowTitle.c_str();
			dialog.lpstrDefExt = defaultExtension.c_str();
			dialog.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);

			const BOOL accepted = save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog);
			if (!accepted) {
				return std::nullopt;
			}
			return std::filesystem::path(buffer.data());
		}
	}

	bool FileDialog::IsAvailable()
	{
		return true;
	}

	std::optional<std::filesystem::path> FileDialog::Open(const std::string& title, const std::string& description, const std::string& extension, const std::filesystem::path& start)
	{
		return ShowFileDialog(false, title, description, extension, start);
	}

	std::optional<std::filesystem::path> FileDialog::Save(const std::string& title, const std::string& description, const std::string& extension, const std::filesystem::path& start)
	{
		const std::optional<std::filesystem::path> path = ShowFileDialog(true, title, description, extension, start);
		if (!path) {
			return std::nullopt;
		}
		return WithExtension(*path, extension);
	}

	std::optional<std::filesystem::path> FileDialog::Folder(const std::string& title, const std::filesystem::path& start)
	{
		const HRESULT initialised = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

		std::optional<std::filesystem::path> result;
		IFileOpenDialog* dialog = nullptr;
		if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dialog)))) {
			DWORD options = 0;
			dialog->GetOptions(&options);
			dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
			dialog->SetTitle(Widen(title).c_str());

			std::error_code error;
			if (!start.empty() && std::filesystem::is_directory(start, error)) {
				IShellItem* folder = nullptr;
				if (SUCCEEDED(SHCreateItemFromParsingName(start.wstring().c_str(), nullptr, IID_PPV_ARGS(&folder)))) {
					dialog->SetFolder(folder);
					folder->Release();
				}
			}

			if (SUCCEEDED(dialog->Show(GetActiveWindow()))) {
				IShellItem* item = nullptr;
				if (SUCCEEDED(dialog->GetResult(&item))) {
					PWSTR chosen = nullptr;
					if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &chosen))) {
						result = std::filesystem::path(chosen);
						CoTaskMemFree(chosen);
					}
					item->Release();
				}
			}
			dialog->Release();
		}

		if (SUCCEEDED(initialised)) {
			CoUninitialize();
		}
		return result;
	}
#else
	namespace
	{
		enum class Tool : std::uint8_t
		{
			Unknown,
			Zenity,
			Kdialog,
			None
		};

		bool HasCommand(const char* name)
		{
			const std::string probe = std::format("command -v {} >/dev/null 2>&1", name);
			return std::system(probe.c_str()) == 0;
		}

		Tool FindTool()
		{
			static Tool tool = Tool::Unknown;
			if (tool == Tool::Unknown) {
				if (HasCommand("zenity")) {
					tool = Tool::Zenity;
				} else if (HasCommand("kdialog")) {
					tool = Tool::Kdialog;
				} else {
					tool = Tool::None;
				}
			}
			return tool;
		}

		std::string Quote(const std::string& text)
		{
			std::string result = "'";
			for (const char c : text) {
				if (c == '\'') {
					result += "'\\''";
				} else {
					result.push_back(c);
				}
			}
			result.push_back('\'');
			return result;
		}

		std::optional<std::filesystem::path> Run(const std::string& command)
		{
			FILE* pipe = popen(command.c_str(), "r");
			if (!pipe) {
				return std::nullopt;
			}

			std::string output;
			std::array<char, 512> chunk = {};
			while (std::fgets(chunk.data(), static_cast<int>(chunk.size()), pipe)) {
				output += chunk.data();
			}

			if (pclose(pipe) != 0) {
				return std::nullopt;
			}

			while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
				output.pop_back();
			}
			if (output.empty()) {
				return std::nullopt;
			}
			return std::filesystem::path(output);
		}

		std::optional<std::filesystem::path> Show(bool save, bool folder, const std::string& title, const std::string& description, const std::string& extension, const std::filesystem::path& start)
		{
			const Tool tool = FindTool();
			if (tool == Tool::None) {
				LOG_WARN("No file dialog found. Install zenity or kdialog.");
				return std::nullopt;
			}

			std::error_code error;
			std::string startText = start.string();
			if (folder && !startText.empty() && startText.back() != '/') {
				startText.push_back('/');
			}
			if (!save && !folder && std::filesystem::is_directory(start, error) && !startText.empty() && startText.back() != '/') {
				startText.push_back('/');
			}

			std::string command;
			if (tool == Tool::Zenity) {
				command = "zenity --file-selection --title=" + Quote(title);
				if (save) {
					command += " --save --confirm-overwrite";
				}
				if (folder) {
					command += " --directory";
				}
				if (!startText.empty()) {
					command += " --filename=" + Quote(startText);
				}
				if (!folder && !extension.empty()) {
					command += " --file-filter=" + Quote(std::format("{} | *{}", description, extension));
				}
			} else {
				const std::string mode = folder ? "--getexistingdirectory" : (save ? "--getsavefilename" : "--getopenfilename");
				command = "kdialog --title " + Quote(title) + " " + mode + " " + Quote(startText.empty() ? std::string(".") : startText);
				if (!folder && !extension.empty()) {
					command += " " + Quote(std::format("*{}|{}", extension, description));
				}
			}
			command += " 2>/dev/null";
			return Run(command);
		}
	}

	bool FileDialog::IsAvailable()
	{
		return FindTool() != Tool::None;
	}

	std::optional<std::filesystem::path> FileDialog::Open(const std::string& title, const std::string& description, const std::string& extension, const std::filesystem::path& start)
	{
		return Show(false, false, title, description, extension, start);
	}

	std::optional<std::filesystem::path> FileDialog::Save(const std::string& title, const std::string& description, const std::string& extension, const std::filesystem::path& start)
	{
		const std::optional<std::filesystem::path> path = Show(true, false, title, description, extension, start);
		if (!path) {
			return std::nullopt;
		}
		return WithExtension(*path, extension);
	}

	std::optional<std::filesystem::path> FileDialog::Folder(const std::string& title, const std::filesystem::path& start)
	{
		return Show(false, true, title, "", "", start);
	}
#endif
}
