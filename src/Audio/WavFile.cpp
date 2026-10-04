#include "pch.h"
#include "Audio/WavFile.h"

namespace SimpleSynthStudio
{
	namespace
	{
		void Put16(std::vector<std::uint8_t>& bytes, std::uint16_t value)
		{
			bytes.push_back(static_cast<std::uint8_t>(value & 0xFF));
			bytes.push_back(static_cast<std::uint8_t>(value >> 8));
		}

		void Put32(std::vector<std::uint8_t>& bytes, std::uint32_t value)
		{
			Put16(bytes, static_cast<std::uint16_t>(value & 0xFFFF));
			Put16(bytes, static_cast<std::uint16_t>(value >> 16));
		}

		void PutText(std::vector<std::uint8_t>& bytes, const char* text)
		{
			for (std::size_t i = 0; text[i] != '\0'; i++) {
				bytes.push_back(static_cast<std::uint8_t>(text[i]));
			}
		}
	}

	std::vector<std::uint8_t> WavFile::Encode(const std::vector<std::int16_t>& samples, std::uint32_t sampleRate)
	{
		constexpr std::uint16_t CHANNELS = 1;
		constexpr std::uint16_t BITS_PER_SAMPLE = 16;
		constexpr std::uint16_t BLOCK_ALIGN = CHANNELS * BITS_PER_SAMPLE / 8;

		const std::uint32_t dataSize = static_cast<std::uint32_t>(samples.size() * sizeof(std::int16_t));

		std::vector<std::uint8_t> bytes;
		bytes.reserve(44 + dataSize);
		PutText(bytes, "RIFF");
		Put32(bytes, 36 + dataSize);
		PutText(bytes, "WAVEfmt ");
		Put32(bytes, 16);
		Put16(bytes, 1);
		Put16(bytes, CHANNELS);
		Put32(bytes, sampleRate);
		Put32(bytes, sampleRate * BLOCK_ALIGN);
		Put16(bytes, BLOCK_ALIGN);
		Put16(bytes, BITS_PER_SAMPLE);
		PutText(bytes, "data");
		Put32(bytes, dataSize);
		for (const std::int16_t sample : samples) {
			Put16(bytes, static_cast<std::uint16_t>(sample));
		}
		return bytes;
	}

	bool WavFile::Write(const std::filesystem::path& path, const std::vector<std::int16_t>& samples, std::uint32_t sampleRate)
	{
		std::ofstream stream(path, std::ios::binary);
		if (!stream) {
			LOG_ERROR("Failed to open {} for writing.", path.string());
			return false;
		}

		const std::vector<std::uint8_t> bytes = Encode(samples, sampleRate);
		stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
		return stream.good();
	}
}
