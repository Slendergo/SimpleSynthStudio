#pragma once

#include "Audio/Synth.h"

namespace SimpleSynthStudio
{
	struct TimelineClip
	{
		SynthVoice Voice;
		float Start = 0.0f;
		float X = 0.0f;
		float Z = -8.0f;

		float GetLength() const { return Voice.Duration + Voice.Release; }
	};

	struct CustomEffect
	{
		std::string Name;
		std::vector<TimelineClip> Clips;

		float GetLength() const
		{
			float length = 0.0f;
			for (const TimelineClip& clip : Clips) {
				length = std::max(length, clip.Start + clip.GetLength());
			}
			return length;
		}
	};

	struct SynthDocument
	{
		std::vector<TimelineClip> Clips;
		std::vector<CustomEffect> Library;
		int Selected = -1;
		bool Repeat = false;
		int Active = -1;

		float GetLength() const
		{
			float length = 0.0f;
			for (const TimelineClip& clip : Clips) {
				length = std::max(length, clip.Start + clip.GetLength());
			}
			return length;
		}
	};
}
