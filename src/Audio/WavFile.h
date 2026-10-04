#pragma once

namespace SimpleSynthStudio
{
	class WavFile
	{
	public:
		static std::vector<std::uint8_t> Encode(const std::vector<std::int16_t>& samples, std::uint32_t sampleRate);
		static bool Write(const std::filesystem::path& path, const std::vector<std::int16_t>& samples, std::uint32_t sampleRate);
	};
}
