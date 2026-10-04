#pragma once

#include "Audio/AudioEngine.h"
#include "Audio/SynthLibrary.h"
#include "Audio/ZipFile.h"

#include "SimpleSynthStudio/SynthDocument.h"
#include "SimpleSynthStudio/SynthHistory.h"

namespace SimpleSynthStudio
{
	struct ScheduledSound
	{
		std::shared_ptr<AudioBuffer> Buffer;
		float Start = 0.0f;
		Vec3 Position;
		bool Fired = false;
	};

	enum class ProjectCommand : std::uint8_t
	{
		None,
		New,
		Open,
		Save,
		SaveAs,
		Exit,
		ExportWav,
		ExportFolder,
		ExportZip,
		ExportProject
	};

	class SynthEditor
	{
	public:
		static constexpr float PAD_RANGE = 20.0f;
		static constexpr float PAD_MIN_SIZE = 120.0f;
		static constexpr float PLACEMENT_PAD_FRACTION = 0.55f;
		static constexpr float PLACEMENT_CONTROLS_WIDTH = 300.0f;
		static constexpr float PLACEMENT_WIDE_WIDTH = 560.0f;
		static constexpr float PLACEMENT_QUICK_DISTANCE = 8.0f;
		static constexpr float TIMELINE_RULER = 22.0f;
		static constexpr float TIMELINE_ROW = 30.0f;
		static constexpr float TIMELINE_MIN_HEIGHT = 150.0f;
		static constexpr float TIMELINE_FRACTION = 0.3f;
		static constexpr float VOICE_FRACTION = 0.27f;
		static constexpr float LIST_FRACTION = 0.2f;
		static constexpr float PREVIEW_FRACTION = 0.16f;
		static constexpr float SNAP_PIXELS = 8.0f;
		static constexpr std::array<float, 6> SNAP_STEPS = { 0.0f, 0.01f, 0.05f, 0.1f, 0.25f, 0.5f };

		SynthEditor();

		bool IsExitConfirmed() const { return _exitConfirmed; }

		bool Initialize();
		void Update(float dt);
		void Draw();

		void OpenProject(const std::filesystem::path& path);
		void RequestExit();
		bool HasUnsavedChanges() const;
		std::string GetTitle() const;

	private:
		void DrawMenuBar();
		void DrawUnsavedPopup();
		void DrawCustomLibrary();
		void DrawRenamePopup();
		void DrawToolbar();
		void DrawTimeline();
		void DrawVoicePanel(TimelineClip& clip);
		void DrawPreviewPanel();
		void DrawPlacementPanel(TimelineClip& clip);
		void DrawLibraryPanel();
		void DrawEffectMenu(float start);
		void LoadEffect(const SynthEffect& effect, float start, bool replace);
		void RequestSwap(const SynthEffect* effect, int custom, bool duplicate = false);
		void PerformSwap(const SynthEffect* effect, int custom, bool duplicate);
		void DrawSwapPopup();
		void SyncActive();
		bool IsReadOnly() const { return _viewTemplate != nullptr; }
		bool IsChecked(const std::string& name) const;
		void SetChecked(const std::string& name, bool checked);
		void ExportSelectedZip(const std::filesystem::path& path);
		bool HasUnboundWork() const;
		std::string MakeUniqueName(const std::string& base) const;
		void LoadClips(const std::vector<TimelineClip>& clips, float start, bool replace);
		void SaveToLibrary(const std::string& name);
		void UpdateLibraryEffect(std::size_t index);
		void ExportEffect(std::size_t index, const std::filesystem::path& path);
		void ExportProjectZip(const std::filesystem::path& path);
		void ExportProjectFolder(const std::filesystem::path& folder);

		void Track();
		void PushUndo();
		void Undo();
		void Redo();
		float SnapStart(float start, int index) const;
		void Select(int index);
		void AddClip(SynthPreset preset, float start);
		void DuplicateClip();
		void DeleteClip();
		TimelineClip* GetSelectedClip();

		void RefreshPreview();
		void PlayClip();
		void StartTimeline();
		void BuildSchedule();
		void StopTimeline();

		void Queue(ProjectCommand command);
		void RunCommand(ProjectCommand command, bool confirmed = false);
		void ResetDocument();
		bool SaveProject();
		bool SaveProjectAs();
		bool EnsureDialogs();

	private:
		AudioEngine _audio;
		SynthDocument _document;
		SynthDocument _frameStart;
		SynthDocument _pending;
		SynthHistory _history;
		AudioPlayProperties _properties;

		std::vector<std::int16_t> _preview;
		bool _dirty = true;
		bool _audition = true;
		bool _playClipRequested = false;
		bool _playTimelineRequested = false;

		float _pixelsPerSecond = 600.0f;
		int _snapIndex = 3;
		bool _snapToClips = true;
		float _contextTime = 0.0f;
		float _dragOrigin = 0.0f;
		bool _dragChanged = false;
		bool _padChanged = false;

		bool _playing = false;
		float _playhead = 0.0f;
		std::vector<ScheduledSound> _schedule;

		std::array<char, 64> _filter = {};
		std::string _status;

		std::filesystem::path _projectPath;
		std::string _savedText;
		ProjectCommand _queued = ProjectCommand::None;
		ProjectCommand _confirming = ProjectCommand::None;
		bool _showUnsavedPopup = false;
		bool _exitConfirmed = false;
		std::filesystem::path _lastDirectory;
		bool _saveChosen = false;
		bool _discardChosen = false;
		const SynthEffect* _swapTemplate = nullptr;
		int _swapCustom = -1;
		bool _swapDuplicate = false;
		const SynthEffect* _viewTemplate = nullptr;
		std::vector<std::string> _checkedSounds;
		std::array<char, 64> _soundFilter = {};
		bool _showSwapPopup = false;
		std::string _pristineText;
		SynthEffect _blankEffect;
		std::size_t _exportIndex = 0;
		std::size_t _renameIndex = 0;
		bool _openRename = false;
		std::array<char, 128> _effectName = {};
		std::array<char, 128> _renameBuffer = {};
	};
}
