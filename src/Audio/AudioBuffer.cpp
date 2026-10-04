#include "pch.h"
#include "Audio/AudioBuffer.h"

#include <AL/al.h>

namespace SimpleSynthStudio
{
	AudioBuffer::AudioBuffer(std::uint32_t handle, float duration)
		: _handle(handle), _duration(duration)
	{
	}

	AudioBuffer::~AudioBuffer()
	{
		if (_handle == 0) {
			return;
		}

		const ALuint handle = _handle;
		alDeleteBuffers(1, &handle);
	}
}
