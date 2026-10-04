#pragma once

namespace SimpleSynthStudio
{
	class AudioBuffer
	{
	public:
		AudioBuffer(std::uint32_t handle, float duration);
		~AudioBuffer();

		AudioBuffer(const AudioBuffer&) = delete;
		AudioBuffer& operator=(const AudioBuffer&) = delete;

		std::uint32_t GetHandle() const { return _handle; }
		float GetDuration() const { return _duration; }

	private:
		std::uint32_t _handle = 0;
		float _duration = 0.0f;
	};
}
