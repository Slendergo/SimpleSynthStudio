#include "pch.h"
#include "SimpleSynthStudio/SynthHistory.h"

namespace SimpleSynthStudio
{
	void SynthHistory::Clear()
	{
		_undo.clear();
		_redo.clear();
	}

	void SynthHistory::Push(const SynthDocument& before)
	{
		_undo.push_back(before);
		if (_undo.size() > MAX_STEPS) {
			_undo.erase(_undo.begin());
		}
		_redo.clear();
	}

	bool SynthHistory::Undo(SynthDocument& document)
	{
		if (_undo.empty()) {
			return false;
		}

		_redo.push_back(document);
		document = std::move(_undo.back());
		_undo.pop_back();
		return true;
	}

	bool SynthHistory::Redo(SynthDocument& document)
	{
		if (_redo.empty()) {
			return false;
		}

		_undo.push_back(document);
		document = std::move(_redo.back());
		_redo.pop_back();
		return true;
	}
}
