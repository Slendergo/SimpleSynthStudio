#pragma once

#define LOG_INFO(message, ...) ::SimpleSynthStudio::Log::Info(message __VA_OPT__(,) __VA_ARGS__)
#define LOG_WARN(message, ...) ::SimpleSynthStudio::Log::Warn(message __VA_OPT__(,) __VA_ARGS__)
#define LOG_ERROR(message, ...) ::SimpleSynthStudio::Log::Error(message __VA_OPT__(,) __VA_ARGS__)

namespace SimpleSynthStudio::Log
{
	enum class Level : std::uint8_t
	{
		Info,
		Warning,
		Error
	};

	inline void Write(Level level, const std::string& message)
	{
		const auto now = std::chrono::system_clock::now();
		const std::time_t time = std::chrono::system_clock::to_time_t(now);
		std::tm local = {};
#ifdef _WIN32
		localtime_s(&local, &time);
#else
		localtime_r(&time, &local);
#endif
		const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

		const char* label = "I";
		const char* colour = "\033[37m";
		switch (level) {
		case Level::Info:
			break;
		case Level::Warning:
			label = "W";
			colour = "\033[33m";
			break;
		case Level::Error:
			label = "E";
			colour = "\033[31m";
			break;
		}

		std::cout << std::format("{}{:02}:{:02}:{:02}.{:03} [{}] -> {}\033[0m\n", colour, local.tm_hour, local.tm_min, local.tm_sec, milliseconds.count(), label, message);
	}

	template<typename... Args>
	void Info(std::format_string<Args...> format, Args&&... args)
	{
		Write(Level::Info, std::format(format, std::forward<Args>(args)...));
	}

	template<typename... Args>
	void Warn(std::format_string<Args...> format, Args&&... args)
	{
		Write(Level::Warning, std::format(format, std::forward<Args>(args)...));
	}

	template<typename... Args>
	void Error(std::format_string<Args...> format, Args&&... args)
	{
		Write(Level::Error, std::format(format, std::forward<Args>(args)...));
	}
}
