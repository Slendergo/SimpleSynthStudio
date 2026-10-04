#pragma once

#include "SimpleSynthStudio/SynthDocument.h"

namespace SimpleSynthStudio
{
	class SynthHistory
	{
	public:
		static constexpr std::size_t MAX_STEPS = 200;

		bool CanUndo() const { return !_undo.empty(); }
		bool CanRedo() const { return !_redo.empty(); }

		void Clear();
		void Push(const SynthDocument& before);
		bool Undo(SynthDocument& document);
		bool Redo(SynthDocument& document);

	private:
		std::vector<SynthDocument> _undo;
		std::vector<SynthDocument> _redo;
	};
}
