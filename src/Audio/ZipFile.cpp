#include "pch.h"
#include "Audio/ZipFile.h"

namespace SimpleSynthStudio
{
	namespace
	{
		constexpr std::uint16_t UTF8_FLAG = 0x0800;
		constexpr std::uint16_t DOS_DATE = 0x0021;
		constexpr std::uint16_t VERSION = 20;

		std::array<std::uint32_t, 256> BuildCrcTable()
		{
			std::array<std::uint32_t, 256> table = {};
			for (std::uint32_t i = 0; i < table.size(); i++) {
				std::uint32_t value = i;
				for (int bit = 0; bit < 8; bit++) {
					value = (value & 1u) ? (0xEDB88320u ^ (value >> 1)) : (value >> 1);
				}
				table[i] = value;
			}
			return table;
		}

		std::uint32_t Crc32(const std::vector<std::uint8_t>& data)
		{
			static const std::array<std::uint32_t, 256> table = BuildCrcTable();
			std::uint32_t crc = 0xFFFFFFFFu;
			for (const std::uint8_t byte : data) {
				crc = table[(crc ^ byte) & 0xFFu] ^ (crc >> 8);
			}
			return crc ^ 0xFFFFFFFFu;
		}

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

		void PutName(std::vector<std::uint8_t>& bytes, const std::string& name)
		{
			bytes.insert(bytes.end(), name.begin(), name.end());
		}
	}

	bool ZipFile::Write(const std::filesystem::path& path, const std::vector<ZipEntry>& entries)
	{
		if (entries.size() > 0xFFFF) {
			LOG_ERROR("Too many files for one zip.");
			return false;
		}

		std::vector<std::uint8_t> archive;
		std::vector<std::uint8_t> directory;
		for (const ZipEntry& entry : entries) {
			const std::uint32_t offset = static_cast<std::uint32_t>(archive.size());
			const std::uint32_t crc = Crc32(entry.Data);
			const std::uint32_t size = static_cast<std::uint32_t>(entry.Data.size());
			const std::uint16_t nameLength = static_cast<std::uint16_t>(entry.Name.size());

			Put32(archive, 0x04034B50u);
			Put16(archive, VERSION);
			Put16(archive, UTF8_FLAG);
			Put16(archive, 0);
			Put16(archive, 0);
			Put16(archive, DOS_DATE);
			Put32(archive, crc);
			Put32(archive, size);
			Put32(archive, size);
			Put16(archive, nameLength);
			Put16(archive, 0);
			PutName(archive, entry.Name);
			archive.insert(archive.end(), entry.Data.begin(), entry.Data.end());

			Put32(directory, 0x02014B50u);
			Put16(directory, VERSION);
			Put16(directory, VERSION);
			Put16(directory, UTF8_FLAG);
			Put16(directory, 0);
			Put16(directory, 0);
			Put16(directory, DOS_DATE);
			Put32(directory, crc);
			Put32(directory, size);
			Put32(directory, size);
			Put16(directory, nameLength);
			Put16(directory, 0);
			Put16(directory, 0);
			Put16(directory, 0);
			Put16(directory, 0);
			Put32(directory, 0);
			Put32(directory, offset);
			PutName(directory, entry.Name);
		}

		const std::uint32_t directoryOffset = static_cast<std::uint32_t>(archive.size());
		archive.insert(archive.end(), directory.begin(), directory.end());

		Put32(archive, 0x06054B50u);
		Put16(archive, 0);
		Put16(archive, 0);
		Put16(archive, static_cast<std::uint16_t>(entries.size()));
		Put16(archive, static_cast<std::uint16_t>(entries.size()));
		Put32(archive, static_cast<std::uint32_t>(directory.size()));
		Put32(archive, directoryOffset);
		Put16(archive, 0);

		std::ofstream stream(path, std::ios::binary);
		if (!stream) {
			LOG_ERROR("Failed to open {} for writing.", path.string());
			return false;
		}
		stream.write(reinterpret_cast<const char*>(archive.data()), static_cast<std::streamsize>(archive.size()));
		return stream.good();
	}
}
