#include "pch.h"
#include "SimpleSynthStudio/SynthEditor.h"

#include <imgui.h>

#include "Audio/WavFile.h"
#include "Audio/ZipFile.h"
#include "SimpleSynthStudio/FileDialog.h"
#include "SimpleSynthStudio/LibraryExporter.h"
#include "SimpleSynthStudio/ProjectExporter.h"
#include "SimpleSynthStudio/SynthProject.h"

namespace SimpleSynthStudio
{
	namespace
	{
		constexpr std::array<const char*, 6> WAVEFORM_NAMES = { "Sine", "Square", "Saw", "Triangle", "Noise", "Crunch" };
		constexpr std::array<const char*, 5> PRESET_NAMES = { "Laser", "Hit", "Pickup", "Explosion", "Blip" };
		constexpr std::array<SynthPreset, 5> PRESETS = { SynthPreset::Laser,  SynthPreset::Hit, SynthPreset::Pickup, SynthPreset::Explosion, SynthPreset::Blip };
		constexpr std::array<ImU32, 6> WAVEFORM_COLOURS = {
			IM_COL32(80, 170, 230, 255),
			IM_COL32(230, 130, 70, 255),
			IM_COL32(170, 120, 230, 255),
			IM_COL32(90, 200, 130, 255),
			IM_COL32(200, 200, 90, 255),
			IM_COL32(210, 110, 130, 255)
		};

		Vec3 GetClipPosition(const TimelineClip& clip) { return { clip.X, 0.0f, clip.Z }; }
	}

	SynthEditor::SynthEditor()
	{
		ResetDocument();

		SynthLayer blank;
		blank.Voice = Synth::GetPreset(SynthPreset::Blip);
		_blankEffect.Name = "New Sound";
		_blankEffect.Layers.push_back(blank);
	}

	bool SynthEditor::Initialize()
	{
		if (!_audio.Initialize()) {
			return false;
		}

		const Vec3 origin = {};
		const Vec3 forward = { 0.0f, 0.0f, -1.0f };
		const Vec3 up = { 0.0f, 1.0f, 0.0f };
		_audio.SetListener(origin, forward, up, origin);
		return true;
	}

	void SynthEditor::Update(float dt)
	{
		const ImGuiIO& io = ImGui::GetIO();
		if (io.KeyCtrl) {
			if (ImGui::IsKeyPressed(ImGuiKey_N, false)) {
				Queue(ProjectCommand::New);
			}
			if (ImGui::IsKeyPressed(ImGuiKey_O, false)) {
				Queue(ProjectCommand::Open);
			}
			if (ImGui::IsKeyPressed(ImGuiKey_S, false)) {
				Queue(io.KeyShift ? ProjectCommand::SaveAs : ProjectCommand::Save);
			}
		}

		if (_saveChosen || _discardChosen) {
			const ProjectCommand command = _confirming;
			const bool save = _saveChosen;
			_saveChosen = false;
			_discardChosen = false;
			_confirming = ProjectCommand::None;
			if (!save || SaveProject()) {
				RunCommand(command, true);
			}
		}

		if (_queued != ProjectCommand::None) {
			const ProjectCommand command = _queued;
			_queued = ProjectCommand::None;
			RunCommand(command);
		}

		if (!io.WantTextInput) {
			if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
				_playTimelineRequested = true;
			}
			if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && !io.KeyShift) {
				Undo();
			}
			if (io.KeyCtrl && (ImGui::IsKeyPressed(ImGuiKey_Y) || (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)))) {
				Redo();
			}
			if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false)) {
				_frameStart = _document;
				DuplicateClip();
			}
			if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
				_frameStart = _document;
				DeleteClip();
			}
		}

		if (_dirty) {
			RefreshPreview();
		}

		if (_playClipRequested) {
			_playClipRequested = false;
			PlayClip();
		}

		if (_playTimelineRequested) {
			_playTimelineRequested = false;
			StartTimeline();
		}

		if (_playing) {
			for (ScheduledSound& sound : _schedule) {
				if (sound.Fired || sound.Start > _playhead) {
					continue;
				}

				const AudioPlayProperties properties = {
					.Position = sound.Position,
					.Volume = _properties.Volume,
					.Pitch = _properties.Pitch,
					.ReferenceDistance = _properties.ReferenceDistance,
					.MaxDistance = _properties.MaxDistance
				};
				_audio.Play(sound.Buffer, properties);
				sound.Fired = true;
			}

			_playhead += dt;
			if (_playhead > _document.GetLength() + 0.05f) {
				if (_document.Repeat && _document.GetLength() > 0.05f) {
					BuildSchedule();
				} else {
					_playing = false;
				}
			}
		}

		_audio.Update();
	}

	void SynthEditor::Draw()
	{
		_frameStart = _document;

		DrawMenuBar();

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
		ImGui::Begin("SimpleSynthStudio", nullptr, flags);

		DrawUnsavedPopup();
		DrawSwapPopup();
		DrawToolbar();

		if (IsReadOnly()) {
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Viewing template: %s (read-only)", _viewTemplate->Name.c_str());
			ImGui::SameLine();
			if (ImGui::Button("Duplicate to My Sounds")) {
				RequestSwap(_viewTemplate, -1, true);
			}
		}

		DrawPreviewPanel();
		DrawTimeline();

		TimelineClip* clip = GetSelectedClip();

		const float totalWidth = ImGui::GetContentRegionAvail().x;
		const float voiceWidth = std::clamp(totalWidth * VOICE_FRACTION, 260.0f, 420.0f);
		const float listWidth = std::clamp(totalWidth * LIST_FRACTION, 190.0f, 320.0f);

		ImGui::BeginChild("voice", ImVec2(voiceWidth, 0.0f), ImGuiChildFlags_Borders);
		if (clip) {
			ImGui::BeginDisabled(IsReadOnly());
			DrawVoicePanel(*clip);
			ImGui::EndDisabled();
		} else {
			ImGui::TextDisabled("No clip selected.");
		}
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("sounds", ImVec2(listWidth, 0.0f), ImGuiChildFlags_Borders);
		DrawCustomLibrary();
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("templates", ImVec2(listWidth, 0.0f), ImGuiChildFlags_Borders);
		DrawLibraryPanel();
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("placement", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
		if (clip) {
			ImGui::BeginDisabled(IsReadOnly());
			DrawPlacementPanel(*clip);
			ImGui::EndDisabled();
		} else {
			ImGui::TextDisabled("No clip selected.");
		}
		ImGui::EndChild();

		SyncActive();

		ImGui::End();
	}

	void SynthEditor::DrawMenuBar()
	{
		if (!ImGui::BeginMainMenuBar()) {
			return;
		}

		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New", "Ctrl+N")) {
				Queue(ProjectCommand::New);
			}
			if (ImGui::MenuItem("Open...", "Ctrl+O")) {
				Queue(ProjectCommand::Open);
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Save", "Ctrl+S")) {
				Queue(ProjectCommand::Save);
			}
			if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
				Queue(ProjectCommand::SaveAs);
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Export Project as ZIP...")) {
				Queue(ProjectCommand::ExportProject);
			}
			if (ImGui::MenuItem("Export Project to Folder...")) {
				Queue(ProjectCommand::ExportFolder);
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit")) {
				Queue(ProjectCommand::Exit);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo", "Ctrl+Z", false, _history.CanUndo())) {
				Undo();
			}
			if (ImGui::MenuItem("Redo", "Ctrl+Y", false, _history.CanRedo())) {
				Redo();
			}
			ImGui::EndMenu();
		}

		if (!_status.empty()) {
			ImGui::TextDisabled("%s", _status.c_str());
		}
		ImGui::EndMainMenuBar();
	}

	void SynthEditor::DrawUnsavedPopup()
	{
		if (_showUnsavedPopup) {
			ImGui::OpenPopup("Unsaved changes");
			_showUnsavedPopup = false;
		}

		if (!ImGui::BeginPopupModal("Unsaved changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			return;
		}

		ImGui::Text("Save changes to %s?", _projectPath.empty() ? "this project" : _projectPath.filename().string().c_str());
		ImGui::Spacing();

		if (ImGui::Button("Save")) {
			_saveChosen = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Discard")) {
			_discardChosen = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			_confirming = ProjectCommand::None;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	void SynthEditor::OpenProject(const std::filesystem::path& path)
	{
		SynthDocument loaded;
		if (!SynthProject::Load(path, loaded)) {
			_status = std::format("Failed to open {}.", path.filename().string());
			return;
		}

		StopTimeline();
		_document = std::move(loaded);
		_history.Clear();
		_viewTemplate = nullptr;
		_projectPath = path;
		_savedText = SynthProject::Serialize(_document);
		_dirty = true;
		_status = std::format("Opened {}.", path.filename().string());
	}

	void SynthEditor::RequestExit()
	{
		Queue(ProjectCommand::Exit);
	}

	bool SynthEditor::HasUnsavedChanges() const
	{
		return SynthProject::Serialize(_document) != _savedText;
	}

	std::string SynthEditor::GetTitle() const
	{
		const std::string name = _projectPath.empty() ? "Untitled" : _projectPath.filename().string();
		return std::format("{}{} - SimpleSynthStudio", name, HasUnsavedChanges() ? "*" : "");
	}

	void SynthEditor::Queue(ProjectCommand command)
	{
		_queued = command;
	}

	void SynthEditor::RunCommand(ProjectCommand command, bool confirmed)
	{
		const bool needsConfirm = command == ProjectCommand::New || command == ProjectCommand::Open || command == ProjectCommand::Exit;
		if (needsConfirm && !confirmed && HasUnsavedChanges()) {
			_confirming = command;
			_showUnsavedPopup = true;
			return;
		}

		switch (command) {
		case ProjectCommand::New:
			StopTimeline();
			_projectPath.clear();
			_history.Clear();
			ResetDocument();
			_status = "New project.";
			break;
		case ProjectCommand::Open: {
			if (!EnsureDialogs()) {
				break;
			}
			const std::filesystem::path start = _projectPath.empty() ? _lastDirectory : _projectPath.parent_path();
			const std::optional<std::filesystem::path> path = FileDialog::Open("Open project", "Synth projects", SynthProject::EXTENSION, start);
			if (path) {
				_lastDirectory = path->parent_path();
				OpenProject(*path);
			}
			break;
		}
		case ProjectCommand::Save:
			SaveProject();
			break;
		case ProjectCommand::SaveAs:
			SaveProjectAs();
			break;
		case ProjectCommand::ExportWav: {
			if (!EnsureDialogs() || _exportIndex >= _document.Library.size()) {
				break;
			}
			const std::string fileName = LibraryExporter::ToFileName(_document.Library[_exportIndex].Name) + ".wav";
			const std::filesystem::path start = _lastDirectory / fileName;
			const std::optional<std::filesystem::path> path = FileDialog::Save("Export WAV", "WAV audio", ".wav", start);
			if (path) {
				_lastDirectory = path->parent_path();
				ExportEffect(_exportIndex, *path);
			}
			break;
		}
		case ProjectCommand::ExportFolder: {
			if (!EnsureDialogs()) {
				break;
			}
			const std::optional<std::filesystem::path> folder = FileDialog::Folder("Choose export folder", _lastDirectory);
			if (folder) {
				_lastDirectory = *folder;
				ExportProjectFolder(*folder);
			}
			break;
		}
		case ProjectCommand::ExportProject: {
			if (!EnsureDialogs()) {
				break;
			}
			const std::string baseName = _projectPath.empty() ? std::string("project") : _projectPath.stem().string();
			const std::filesystem::path start = _lastDirectory / (baseName + ".zip");
			const std::optional<std::filesystem::path> path = FileDialog::Save("Export project", "ZIP archive", ".zip", start);
			if (path) {
				_lastDirectory = path->parent_path();
				ExportProjectZip(*path);
			}
			break;
		}
		case ProjectCommand::ExportZip: {
			if (!EnsureDialogs()) {
				break;
			}
			const std::string baseName = _projectPath.empty() ? std::string("sounds") : _projectPath.stem().string();
			const std::filesystem::path start = _lastDirectory / (baseName + ".zip");
			const std::optional<std::filesystem::path> path = FileDialog::Save("Export sounds", "ZIP archive", ".zip", start);
			if (path) {
				_lastDirectory = path->parent_path();
				ExportSelectedZip(*path);
			}
			break;
		}
		case ProjectCommand::Exit:
			_exitConfirmed = true;
			break;
		case ProjectCommand::None:
			break;
		}
	}

	void SynthEditor::ResetDocument()
	{
		TimelineClip clip;
		clip.Voice = Synth::GetPreset(SynthPreset::Blip);
		_viewTemplate = nullptr;
		_document = SynthDocument();
		_document.Clips.push_back(clip);
		_document.Selected = 0;
		_pristineText = SynthProject::Serialize(_document);
		_savedText = _pristineText;
		_dirty = true;
	}

	bool SynthEditor::SaveProject()
	{
		if (_projectPath.empty()) {
			return SaveProjectAs();
		}

		if (!SynthProject::Save(_projectPath, _document)) {
			_status = std::format("Failed to save {}.", _projectPath.filename().string());
			return false;
		}

		_savedText = SynthProject::Serialize(_document);
		_status = std::format("Saved {}.", _projectPath.string());
		return true;
	}

	bool SynthEditor::SaveProjectAs()
	{
		if (!EnsureDialogs()) {
			return false;
		}

		const std::filesystem::path directory = _projectPath.empty() ? _lastDirectory : _projectPath.parent_path();
		const std::string name = _projectPath.empty() ? std::string("sound.psynth") : _projectPath.filename().string();
		const std::filesystem::path start = directory / name;
		const std::optional<std::filesystem::path> chosen = FileDialog::Save("Save project", "Synth projects", SynthProject::EXTENSION, start);
		if (!chosen) {
			return false;
		}

		_lastDirectory = chosen->parent_path();
		_projectPath = *chosen;
		return SaveProject();
	}

	bool SynthEditor::EnsureDialogs()
	{
		if (FileDialog::IsAvailable()) {
			return true;
		}

		_status = "No file dialog found. Install zenity or kdialog.";
		return false;
	}

	void SynthEditor::DrawToolbar()
	{
		if (ImGui::Button("Play")) {
			_playTimelineRequested = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Stop")) {
			StopTimeline();
		}
		ImGui::SameLine();
		ImGui::BeginDisabled(IsReadOnly());
		bool repeat = _document.Repeat;
		if (ImGui::Checkbox("Repeat", &repeat)) {
			PushUndo();
			_document.Repeat = repeat;
		}
		ImGui::EndDisabled();

		ImGui::SameLine();
		ImGui::TextDisabled("|");
		ImGui::SameLine();

		ImGui::BeginDisabled(!_history.CanUndo());
		if (ImGui::Button("Undo")) {
			Undo();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(!_history.CanRedo());
		if (ImGui::Button("Redo")) {
			Redo();
		}
		ImGui::EndDisabled();

		ImGui::SameLine();
		ImGui::TextDisabled("|");
		ImGui::SameLine();

		ImGui::BeginDisabled(IsReadOnly());
		if (ImGui::Button("Add clip")) {
			AddClip(SynthPreset::Blip, _document.GetLength());
		}
		ImGui::SameLine();
		ImGui::BeginDisabled(_document.Selected < 0);
		if (ImGui::Button("Duplicate")) {
			DuplicateClip();
		}
		ImGui::SameLine();
		if (ImGui::Button("Delete")) {
			DeleteClip();
		}
		ImGui::EndDisabled();
		ImGui::EndDisabled();

		ImGui::SameLine();
		ImGui::TextDisabled("|");
		ImGui::SameLine();

		ImGui::SetNextItemWidth(90.0f);
		if (ImGui::BeginCombo("Snap", _snapIndex == 0 ? "Off" : std::format("{} s", SNAP_STEPS[static_cast<std::size_t>(_snapIndex)]).c_str())) {
			for (int i = 0; i < static_cast<int>(SNAP_STEPS.size()); i++) {
				const std::string label = i == 0 ? "Off" : std::format("{} s", SNAP_STEPS[static_cast<std::size_t>(i)]);
				if (ImGui::Selectable(label.c_str(), i == _snapIndex)) {
					_snapIndex = i;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		ImGui::Checkbox("Snap to clips", &_snapToClips);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120.0f);
		ImGui::SliderFloat("Zoom", &_pixelsPerSecond, 60.0f, 1200.0f, "%.0f px/s");
		ImGui::SameLine();
		ImGui::Checkbox("Play on edit", &_audition);

	}

	void SynthEditor::DrawTimeline()
	{
		const float timelineHeight = std::max(ImGui::GetContentRegionAvail().y * TIMELINE_FRACTION, TIMELINE_MIN_HEIGHT);
		ImGui::BeginChild("timeline", ImVec2(0.0f, timelineHeight), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);

		const float seconds = std::max(_document.GetLength() + 1.0f, 4.0f);
		const float width = std::max(seconds * _pixelsPerSecond, ImGui::GetContentRegionAvail().x);
		const int rows = std::max(static_cast<int>(_document.Clips.size()) + 1, 4);
		const ImVec2 origin = ImGui::GetCursorScreenPos();
		ImGui::Dummy(ImVec2(width, TIMELINE_RULER + static_cast<float>(rows) * TIMELINE_ROW));

		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			const float time = (ImGui::GetIO().MousePos.x - origin.x) / _pixelsPerSecond;
			_contextTime = SnapStart(time, -1);
		}
		if (!IsReadOnly() && ImGui::BeginPopupContextItem("timeline_menu")) {
			if (ImGui::MenuItem("Add blank clip here")) {
				AddClip(SynthPreset::Blip, _contextTime);
			}
			DrawEffectMenu(_contextTime);
			ImGui::EndPopup();
		}

		ImDrawList* draw = ImGui::GetWindowDrawList();
		for (int row = 0; row < rows; row++) {
			const float top = origin.y + TIMELINE_RULER + static_cast<float>(row) * TIMELINE_ROW;
			const ImU32 colour = row % 2 == 0 ? IM_COL32(28, 28, 34, 255) : IM_COL32(34, 34, 42, 255);
			draw->AddRectFilled(ImVec2(origin.x, top), ImVec2(origin.x + width, top + TIMELINE_ROW), colour);
		}

		const float gridStep = SNAP_STEPS[static_cast<std::size_t>(_snapIndex)];
		if (gridStep > 0.0f && gridStep * _pixelsPerSecond >= 6.0f) {
			const int lines = static_cast<int>(seconds / gridStep);
			const float bottom = origin.y + TIMELINE_RULER + static_cast<float>(rows) * TIMELINE_ROW;
			for (int i = 0; i <= lines; i++) {
				const float x = origin.x + static_cast<float>(i) * gridStep * _pixelsPerSecond;
				draw->AddLine(ImVec2(x, origin.y + TIMELINE_RULER), ImVec2(x, bottom), IM_COL32(255, 255, 255, 14));
			}
		}

		const int ticks = static_cast<int>(seconds * 10.0f);
		for (int i = 0; i <= ticks; i++) {
			const bool major = i % 10 == 0;
			if (!major && _pixelsPerSecond < 120.0f) {
				continue;
			}

			const float x = origin.x + static_cast<float>(i) * 0.1f * _pixelsPerSecond;
			const float top = origin.y + (major ? 2.0f : 12.0f);
			draw->AddLine(ImVec2(x, top), ImVec2(x, origin.y + TIMELINE_RULER), IM_COL32(120, 120, 130, 255));
			if (major) {
				const std::string label = std::format("{}s", i / 10);
				draw->AddText(ImVec2(x + 3.0f, origin.y + 2.0f), IM_COL32(170, 170, 180, 255), label.c_str());
			}
		}

		for (int i = 0; i < static_cast<int>(_document.Clips.size()); i++) {
			TimelineClip& clip = _document.Clips[static_cast<std::size_t>(i)];
			const float rowTop = origin.y + TIMELINE_RULER + static_cast<float>(i) * TIMELINE_ROW + 3.0f;
			const float rowHeight = TIMELINE_ROW - 6.0f;
			const float heldWidth = std::max(clip.Voice.Duration * _pixelsPerSecond, 6.0f);
			const float totalWidth = std::max(clip.GetLength() * _pixelsPerSecond, 8.0f);

			ImGui::SetCursorScreenPos(ImVec2(origin.x + clip.Start * _pixelsPerSecond, rowTop));
			ImGui::PushID(i);
			ImGui::InvisibleButton("clip", ImVec2(totalWidth, rowHeight));

			if (ImGui::IsItemActivated()) {
				_pending = _frameStart;
				_dragOrigin = clip.Start;
				_dragChanged = false;
				if (_document.Selected != i) {
					Select(i);
				}
			}
			if (ImGui::IsItemActive() && !IsReadOnly()) {
				const float delta = ImGui::GetMouseDragDelta(0, 0.0f).x / _pixelsPerSecond;
				const float snapped = SnapStart(_dragOrigin + delta, i);
				if (snapped != clip.Start) {
					clip.Start = snapped;
					_dragChanged = true;
				}
			}
			if (ImGui::IsItemDeactivated() && _dragChanged) {
				_history.Push(_pending);
				_dragChanged = false;
			}
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && _document.Selected != i) {
				Select(i);
			}
			if (!IsReadOnly() && ImGui::BeginPopupContextItem("clip_menu")) {
				if (ImGui::MenuItem("Play")) {
					_playClipRequested = true;
				}
				if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
					DuplicateClip();
				}
				if (ImGui::MenuItem("Delete", "Del")) {
					DeleteClip();
				}
				if (ImGui::BeginMenu("Replace with")) {
					for (std::size_t preset = 0; preset < PRESETS.size(); preset++) {
						if (ImGui::MenuItem(PRESET_NAMES[preset])) {
							PushUndo();
							clip.Voice = Synth::GetPreset(PRESETS[preset]);
							_dirty = true;
							_playClipRequested = _audition;
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}
			ImGui::PopID();

			const float left = origin.x + clip.Start * _pixelsPerSecond;
			const ImU32 colour = WAVEFORM_COLOURS[static_cast<std::size_t>(clip.Voice.Wave)];
			const ImU32 tailColour = (colour & 0x00FFFFFF) | 0x80000000;
			draw->AddRectFilled(ImVec2(left, rowTop), ImVec2(left + totalWidth, rowTop + rowHeight), tailColour, 3.0f);
			draw->AddRectFilled(ImVec2(left, rowTop), ImVec2(left + heldWidth, rowTop + rowHeight), colour, 3.0f);
			draw->AddText(ImVec2(left + 4.0f, rowTop + 4.0f), IM_COL32(10, 10, 14, 255), WAVEFORM_NAMES[static_cast<std::size_t>(clip.Voice.Wave)]);
			if (_document.Selected == i) {
				draw->AddRect(ImVec2(left, rowTop), ImVec2(left + totalWidth, rowTop + rowHeight), IM_COL32(255, 255, 255, 255), 3.0f, 0, 2.0f);
			}
		}

		if (_playing) {
			const float x = origin.x + _playhead * _pixelsPerSecond;
			draw->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + TIMELINE_RULER + static_cast<float>(rows) * TIMELINE_ROW), IM_COL32(255, 80, 80, 255), 2.0f);
		}

		ImGui::EndChild();
	}

	void SynthEditor::DrawVoicePanel(TimelineClip& clip)
	{
		SynthVoice& voice = clip.Voice;

		ImGui::SeparatorText("Clip");
		ImGui::SliderFloat("Start", &clip.Start, 0.0f, 10.0f, "%.2f s");
		Track();

		ImGui::SeparatorText("Oscillator");
		int wave = static_cast<int>(voice.Wave);
		if (ImGui::Combo("Waveform", &wave, WAVEFORM_NAMES.data(), static_cast<int>(WAVEFORM_NAMES.size()))) {
			PushUndo();
			voice.Wave = static_cast<Waveform>(wave);
			_dirty = true;
			_playClipRequested = _audition;
		}

		ImGui::SliderFloat("Start Hz", &voice.Frequency, 20.0f, 8000.0f, "%.0f", ImGuiSliderFlags_Logarithmic);
		Track();
		ImGui::SliderFloat("End Hz", &voice.EndFrequency, 20.0f, 8000.0f, "%.0f", ImGuiSliderFlags_Logarithmic);
		Track();
		ImGui::SliderFloat("Vibrato rate", &voice.VibratoRate, 0.0f, 30.0f, "%.1f Hz");
		Track();
		ImGui::SliderFloat("Vibrato depth", &voice.VibratoDepth, 0.0f, 0.5f, "%.2f");
		Track();
		ImGui::SliderFloat("Pulse width", &voice.PulseWidth, 0.05f, 0.95f, "%.2f");
		Track();

		ImGui::SeparatorText("Filter");
		ImGui::SliderFloat("Cutoff", &voice.Cutoff, 0.0f, 16000.0f, "%.0f (0 = off)", ImGuiSliderFlags_Logarithmic);
		Track();
		ImGui::SliderFloat("Cutoff end", &voice.EndCutoff, 0.0f, 16000.0f, "%.0f (0 = same)", ImGuiSliderFlags_Logarithmic);
		Track();

		ImGui::SeparatorText("Envelope");
		ImGui::SliderFloat("Length", &voice.Duration, 0.01f, 2.0f, "%.2f s");
		Track();
		ImGui::SliderFloat("Attack", &voice.Attack, 0.0f, 1.0f, "%.3f s", ImGuiSliderFlags_Logarithmic);
		Track();
		ImGui::SliderFloat("Decay", &voice.Decay, 0.0f, 1.0f, "%.3f s", ImGuiSliderFlags_Logarithmic);
		Track();
		ImGui::SliderFloat("Sustain", &voice.Sustain, 0.0f, 1.0f, "%.2f");
		Track();
		ImGui::SliderFloat("Release", &voice.Release, 0.0f, 2.0f, "%.3f s", ImGuiSliderFlags_Logarithmic);
		Track();
		ImGui::SliderFloat("Volume", &voice.Volume, 0.0f, 1.0f, "%.2f");
		Track();
	}

	void SynthEditor::DrawPreviewPanel()
	{
		ImGui::SeparatorText("Selected clip waveform");

		const ImVec2 available = ImGui::GetContentRegionAvail();
		const ImVec2 size = ImVec2(available.x, std::clamp(available.y * PREVIEW_FRACTION, 56.0f, 110.0f));
		const ImVec2 origin = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("preview", size);

		ImDrawList* draw = ImGui::GetWindowDrawList();
		const ImVec2 corner = ImVec2(origin.x + size.x, origin.y + size.y);
		draw->AddRectFilled(origin, corner, IM_COL32(20, 20, 26, 255));

		const float middle = origin.y + size.y * 0.5f;
		const float half = size.y * 0.5f - 2.0f;
		draw->AddLine(ImVec2(origin.x, middle), ImVec2(corner.x, middle), IM_COL32(60, 60, 70, 255));

		const std::size_t count = _preview.size();
		const std::size_t columns = static_cast<std::size_t>(size.x);
		if (count == 0 || columns == 0) {
			return;
		}

		for (std::size_t column = 0; column < columns; column++) {
			const std::size_t begin = column * count / columns;
			const std::size_t end = std::min(std::max(begin + 1, (column + 1) * count / columns), count);

			std::int16_t low = 0;
			std::int16_t high = 0;
			for (std::size_t i = begin; i < end; i++) {
				low = std::min(low, _preview[i]);
				high = std::max(high, _preview[i]);
			}

			const float x = origin.x + static_cast<float>(column);
			const float top = middle - static_cast<float>(high) / 32768.0f * half;
			const float bottom = middle - static_cast<float>(low) / 32768.0f * half;
			draw->AddLine(ImVec2(x, top), ImVec2(x, bottom + 1.0f), IM_COL32(110, 200, 255, 255));
		}
	}

	void SynthEditor::DrawPlacementPanel(TimelineClip& clip)
	{
		ImGui::SeparatorText("Audio Placement");
		ImGui::TextWrapped("Top-down view. You are the listener in the middle.");

		const ImVec2 available = ImGui::GetContentRegionAvail();
		const bool sideBySide = available.x >= PLACEMENT_WIDE_WIDTH;
		const float padLimit = sideBySide ? std::min(available.y - 4.0f, available.x - PLACEMENT_CONTROLS_WIDTH) : std::min(available.x - 4.0f, available.y * PLACEMENT_PAD_FRACTION);
		const float padSize = std::max(padLimit, PAD_MIN_SIZE);

		ImGui::BeginGroup();
		const ImVec2 size = ImVec2(padSize, padSize);
		const ImVec2 origin = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("pad", size);
		const bool hovered = ImGui::IsItemHovered();

		const ImVec2 corner = ImVec2(origin.x + size.x, origin.y + size.y);
		const ImVec2 centre = ImVec2(origin.x + size.x * 0.5f, origin.y + size.y * 0.5f);
		const float scale = size.x * 0.5f / PAD_RANGE;

		if (ImGui::IsItemActivated()) {
			_pending = _frameStart;
			_padChanged = false;
		}
		if (ImGui::IsItemActive()) {
			const ImVec2 mouse = ImGui::GetIO().MousePos;
			float x = std::clamp((mouse.x - centre.x) / scale, -PAD_RANGE, PAD_RANGE);
			float z = std::clamp((mouse.y - centre.y) / scale, -PAD_RANGE, PAD_RANGE);
			if (ImGui::GetIO().KeyShift) {
				x = std::round(x);
				z = std::round(z);
			}
			if (x != clip.X || z != clip.Z) {
				clip.X = x;
				clip.Z = z;
				_padChanged = true;
			}
		}
		if (ImGui::IsItemDeactivated() && _padChanged) {
			_history.Push(_pending);
			_padChanged = false;
			_playClipRequested = _audition;
		}

		const ImU32 lineColour = IM_COL32(120, 122, 130, 255);
		const ImU32 faintColour = IM_COL32(70, 72, 80, 255);
		const ImU32 textColour = IM_COL32(150, 152, 160, 255);
		const ImU32 whiteColour = IM_COL32(230, 230, 235, 255);

		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddRectFilled(origin, corner, IM_COL32(20, 20, 24, 255));
		draw->PushClipRect(origin, corner, true);

		draw->AddLine(ImVec2(origin.x, centre.y), ImVec2(corner.x, centre.y), faintColour);
		draw->AddLine(ImVec2(centre.x, origin.y), ImVec2(centre.x, corner.y), faintColour);

		const float referenceRadius = _properties.ReferenceDistance * scale;
		const float maxRadius = _properties.MaxDistance * scale;
		draw->AddCircle(centre, referenceRadius, lineColour, 64);
		draw->AddCircle(centre, maxRadius, faintColour, 64);
		const std::string referenceLabel = std::format("full {:.0f}", _properties.ReferenceDistance);
		const std::string maxLabel = std::format("silent {:.0f}", _properties.MaxDistance);
		draw->AddText(ImVec2(centre.x + referenceRadius * 0.7071f + 3.0f, centre.y - referenceRadius * 0.7071f - 14.0f), textColour, referenceLabel.c_str());
		draw->AddText(ImVec2(centre.x + maxRadius * 0.7071f + 3.0f, centre.y - maxRadius * 0.7071f - 14.0f), textColour, maxLabel.c_str());

		const ImVec2 frontSize = ImGui::CalcTextSize("FRONT");
		const ImVec2 backSize = ImGui::CalcTextSize("BACK");
		draw->AddText(ImVec2(centre.x - frontSize.x * 0.5f, origin.y + 4.0f), textColour, "FRONT");
		draw->AddText(ImVec2(centre.x - backSize.x * 0.5f, corner.y - backSize.y - 4.0f), textColour, "BACK");
		draw->AddText(ImVec2(origin.x + 6.0f, centre.y - ImGui::GetTextLineHeight() * 0.5f), textColour, "L");
		draw->AddText(ImVec2(corner.x - ImGui::CalcTextSize("R").x - 6.0f, centre.y - ImGui::GetTextLineHeight() * 0.5f), textColour, "R");

		const ImVec2 selectedDot = ImVec2(centre.x + clip.X * scale, centre.y + clip.Z * scale);
		draw->AddLine(centre, selectedDot, lineColour);

		for (int i = 0; i < static_cast<int>(_document.Clips.size()); i++) {
			if (i == _document.Selected) {
				continue;
			}
			const TimelineClip& other = _document.Clips[static_cast<std::size_t>(i)];
			draw->AddCircleFilled(ImVec2(centre.x + other.X * scale, centre.y + other.Z * scale), 3.0f, lineColour);
		}
		draw->AddCircleFilled(selectedDot, 6.0f, whiteColour);

		const float distance = std::sqrt(clip.X * clip.X + clip.Z * clip.Z);
		if (distance > 1.5f) {
			const std::string distanceLabel = std::format("{:.1f}", distance);
			draw->AddText(ImVec2((centre.x + selectedDot.x) * 0.5f + 6.0f, (centre.y + selectedDot.y) * 0.5f - 14.0f), textColour, distanceLabel.c_str());
		}

		draw->AddTriangleFilled(ImVec2(centre.x, centre.y - 8.0f), ImVec2(centre.x - 5.0f, centre.y + 5.0f), ImVec2(centre.x + 5.0f, centre.y + 5.0f), whiteColour);
		draw->AddText(ImVec2(centre.x + 9.0f, centre.y + 2.0f), textColour, "You");

		draw->PopClipRect();
		draw->AddRect(origin, corner, faintColour);

		if (hovered) {
			ImGui::SetTooltip("Drag to place the sound.\nHold Shift to snap to whole units.");
		}

		const char* sideName = clip.X < 0.0f ? "left" : "right";
		const char* depthName = clip.Z < 0.0f ? "front" : "behind";
		ImGui::Text("%.1f away: %.1f %s, %.1f %s", distance, std::abs(clip.X), sideName, std::abs(clip.Z), depthName);
		ImGui::EndGroup();

		if (sideBySide) {
			ImGui::SameLine();
		}

		ImGui::BeginGroup();
		ImGui::SeparatorText("Position");
		ImGui::SetNextItemWidth(-110.0f);
		ImGui::DragFloat("Left / Right", &clip.X, 0.1f, -PAD_RANGE, PAD_RANGE, "%.1f");
		Track();
		ImGui::SetNextItemWidth(-110.0f);
		ImGui::DragFloat("Front / Back", &clip.Z, 0.1f, -PAD_RANGE, PAD_RANGE, "%.1f");
		Track();

		const auto place = [this, &clip](float x, float z)
		{
			PushUndo();
			clip.X = x;
			clip.Z = z;
			_playClipRequested = _audition;
		};

		if (ImGui::Button("Front")) {
			place(0.0f, -PLACEMENT_QUICK_DISTANCE);
		}
		ImGui::SameLine();
		if (ImGui::Button("Left")) {
			place(-PLACEMENT_QUICK_DISTANCE, 0.0f);
		}
		ImGui::SameLine();
		if (ImGui::Button("Right")) {
			place(PLACEMENT_QUICK_DISTANCE, 0.0f);
		}
		ImGui::SameLine();
		if (ImGui::Button("Behind")) {
			place(0.0f, PLACEMENT_QUICK_DISTANCE);
		}

		ImGui::SeparatorText("Playback");
		ImGui::SetNextItemWidth(-110.0f);
		ImGui::SliderFloat("Volume", &_properties.Volume, 0.0f, 1.0f, "%.2f");
		ImGui::SetNextItemWidth(-110.0f);
		ImGui::SliderFloat("Pitch", &_properties.Pitch, 0.25f, 4.0f, "%.2f", ImGuiSliderFlags_Logarithmic);

		ImGui::SeparatorText("Distance falloff");
		ImGui::SetNextItemWidth(-110.0f);
		ImGui::SliderFloat("Full volume to", &_properties.ReferenceDistance, 0.5f, PAD_RANGE, "%.1f");
		ImGui::SetNextItemWidth(-110.0f);
		ImGui::SliderFloat("Silent at", &_properties.MaxDistance, 1.0f, PAD_RANGE * 2.0f, "%.1f");
		_properties.MaxDistance = std::max(_properties.MaxDistance, _properties.ReferenceDistance + 0.5f);

		ImGui::Spacing();
		if (ImGui::Button("Play clip")) {
			_playClipRequested = true;
		}
		ImGui::EndGroup();
	}

	void SynthEditor::DrawCustomLibrary()
	{
		ImGui::SeparatorText("My Sounds");

		const bool bound = _document.Active >= 0 && _document.Active < static_cast<int>(_document.Library.size());
		if (bound) {
			const std::string& name = _document.Library[static_cast<std::size_t>(_document.Active)].Name;
			ImGui::TextColored(ImVec4(0.45f, 0.85f, 0.55f, 1.0f), "Editing  %s", name.c_str());
		} else if (IsReadOnly()) {
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Viewing a template");
		} else {
			ImGui::TextColored(ImVec4(0.6f, 0.75f, 1.0f, 1.0f), "Unsaved timeline");
		}

		const std::size_t total = _document.Library.size();
		const std::string count = std::format("{} sound{}", total, total == 1 ? "" : "s");
		ImGui::SameLine(std::max(ImGui::GetCursorPosX(), ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(count.c_str()).x));
		ImGui::TextDisabled("%s", count.c_str());

		if (ImGui::Button("+ New sound")) {
			RequestSwap(&_blankEffect, -1, true);
		}
		ImGui::SameLine();
		if (ImGui::Button("Export...")) {
			ImGui::OpenPopup("export_menu");
		}
		if (ImGui::BeginPopup("export_menu")) {
			if (ImGui::MenuItem("Project as ZIP (sounds + templates)...")) {
				Queue(ProjectCommand::ExportProject);
			}
			if (ImGui::MenuItem("Project to a folder (sounds + templates)...")) {
				Queue(ProjectCommand::ExportFolder);
			}
			ImGui::Separator();
			const std::string selectedLabel = std::format("Selected sounds as ZIP ({})...", _checkedSounds.size());
			if (ImGui::MenuItem(selectedLabel.c_str(), nullptr, false, !_checkedSounds.empty())) {
				Queue(ProjectCommand::ExportZip);
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Select all", nullptr, false, total > 0)) {
				for (const CustomEffect& effect : _document.Library) {
					SetChecked(effect.Name, true);
				}
			}
			if (ImGui::MenuItem("Select none", nullptr, false, !_checkedSounds.empty())) {
				_checkedSounds.clear();
			}
			ImGui::EndPopup();
		}

		if (!bound) {
			const bool canSave = _effectName[0] != '\0' && !_document.Clips.empty();
			ImGui::SetNextItemWidth(-64.0f);
			ImGui::InputTextWithHint("##effectname", "Name this timeline...", _effectName.data(), _effectName.size());
			ImGui::SameLine();
			ImGui::BeginDisabled(!canSave);
			if (ImGui::Button("Save")) {
				SaveToLibrary(_effectName.data());
			}
			ImGui::EndDisabled();
		}

		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputTextWithHint("##soundfilter", "Search sounds...", _soundFilter.data(), _soundFilter.size());

		if (!_checkedSounds.empty()) {
			ImGui::TextDisabled("%d selected", static_cast<int>(_checkedSounds.size()));
			ImGui::SameLine();
			if (ImGui::SmallButton("Clear")) {
				_checkedSounds.clear();
			}
		}

		const auto lower = [](std::string text)
		{
			std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return text;
		};
		const std::string filter = lower(_soundFilter.data());

		std::optional<std::size_t> deleteIndex;
		std::optional<std::size_t> duplicateIndex;

		ImGui::Spacing();
		ImGui::BeginChild("sound_list", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
		if (total == 0) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
			ImGui::TextWrapped("No sounds yet. Click New sound, or open a template and duplicate it.");
			ImGui::PopStyleColor();
		}

		ImDrawList* draw = ImGui::GetWindowDrawList();
		for (std::size_t i = 0; i < total; i++) {
			const CustomEffect& effect = _document.Library[i];
			if (!filter.empty() && lower(effect.Name).find(filter) == std::string::npos) {
				continue;
			}

			ImGui::PushID(static_cast<int>(i));
			bool checked = IsChecked(effect.Name);
			if (ImGui::Checkbox("##check", &checked)) {
				SetChecked(effect.Name, checked);
			}
			ImGui::SameLine();
			if (ImGui::Selectable(effect.Name.c_str(), static_cast<int>(i) == _document.Active)) {
				RequestSwap(nullptr, static_cast<int>(i));
			}

			const ImVec2 rowMin = ImGui::GetItemRectMin();
			const ImVec2 rowMax = ImGui::GetItemRectMax();
			const std::string length = std::format("{:.2f}s", effect.GetLength());
			const ImVec2 lengthSize = ImGui::CalcTextSize(length.c_str());
			draw->AddText(ImVec2(rowMax.x - lengthSize.x - 6.0f, rowMin.y + (rowMax.y - rowMin.y - lengthSize.y) * 0.5f), IM_COL32(150, 155, 170, 255), length.c_str());

			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%d layer%s, %.2fs\nClick to edit, right-click for more.", static_cast<int>(effect.Clips.size()), effect.Clips.size() == 1 ? "" : "s", effect.GetLength());
			}

			if (ImGui::BeginPopupContextItem("custom_menu")) {
				if (ImGui::MenuItem("Edit")) {
					RequestSwap(nullptr, static_cast<int>(i));
				}
				if (ImGui::MenuItem("Add at end of timeline")) {
					LoadClips(effect.Clips, _document.GetLength(), false);
				}
				if (ImGui::MenuItem("Duplicate")) {
					duplicateIndex = i;
				}
				if (ImGui::MenuItem("Update from timeline", nullptr, false, !_document.Clips.empty())) {
					UpdateLibraryEffect(i);
				}
				if (ImGui::MenuItem("Rename...")) {
					_renameIndex = i;
					_renameBuffer = {};
					std::copy_n(effect.Name.begin(), std::min(effect.Name.size(), _renameBuffer.size() - 1), _renameBuffer.begin());
					_openRename = true;
				}
				if (ImGui::MenuItem("Export WAV...")) {
					_exportIndex = i;
					Queue(ProjectCommand::ExportWav);
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Delete")) {
					deleteIndex = i;
				}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}
		ImGui::EndChild();

		if (duplicateIndex) {
			PushUndo();
			CustomEffect copy = _document.Library[*duplicateIndex];
			copy.Name = MakeUniqueName(copy.Name + " copy");
			_document.Library.push_back(std::move(copy));
		}

		if (deleteIndex) {
			PushUndo();
			SetChecked(_document.Library[*deleteIndex].Name, false);
			_document.Library.erase(_document.Library.begin() + static_cast<std::ptrdiff_t>(*deleteIndex));
			if (_document.Active == static_cast<int>(*deleteIndex)) {
				_document.Active = -1;
			} else if (_document.Active > static_cast<int>(*deleteIndex)) {
				_document.Active--;
			}
		}

		DrawRenamePopup();
	}

	void SynthEditor::DrawRenamePopup()
	{
		if (_openRename) {
			ImGui::OpenPopup("Rename effect");
			_openRename = false;
		}

		if (!ImGui::BeginPopupModal("Rename effect", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			return;
		}

		bool apply = ImGui::InputText("##rename", _renameBuffer.data(), _renameBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue);
		if (ImGui::Button("OK")) {
			apply = true;
		}
		ImGui::SameLine();
		const bool cancel = ImGui::Button("Cancel");

		if (apply && _renameBuffer[0] != '\0' && _renameIndex < _document.Library.size()) {
			PushUndo();
			const std::string oldName = _document.Library[_renameIndex].Name;
			const bool wasChecked = IsChecked(oldName);
			SetChecked(oldName, false);
			_document.Library[_renameIndex].Name = _renameBuffer.data();
			SetChecked(_document.Library[_renameIndex].Name, wasChecked);
		}
		if (apply || cancel) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	void SynthEditor::DrawLibraryPanel()
	{
		ImGui::SeparatorText("Templates (read-only ideas)");
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputTextWithHint("##filter", "Search effects...", _filter.data(), _filter.size());
		ImGui::TextWrapped("Click: view it read-only. Right-click: duplicate it into My Sounds.");

		const auto lower = [](std::string text)
		{
			std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return text;
		};
		const std::string filter = lower(_filter.data());

		const std::vector<SynthEffect>& effects = SynthLibrary::GetEffects();
		for (const std::string& category : SynthLibrary::GetCategories()) {
			std::vector<const SynthEffect*> matches;
			for (const SynthEffect& effect : effects) {
				if (effect.Category != category) {
					continue;
				}
				if (filter.empty() || lower(effect.Name).find(filter) != std::string::npos || lower(category).find(filter) != std::string::npos) {
					matches.push_back(&effect);
				}
			}
			if (matches.empty()) {
				continue;
			}

			if (!filter.empty()) {
				ImGui::SetNextItemOpen(true, ImGuiCond_Always);
			}
			if (!ImGui::CollapsingHeader(category.c_str())) {
				continue;
			}

			for (const SynthEffect* effect : matches) {
				ImGui::PushID(effect);
				if (ImGui::Selectable(effect->Name.c_str(), effect == _viewTemplate)) {
					RequestSwap(effect, -1);
				}
				if (ImGui::BeginPopupContextItem("effect_menu")) {
					if (ImGui::MenuItem("Duplicate to My Sounds")) {
						RequestSwap(effect, -1, true);
					}
					if (ImGui::MenuItem("Add at end of timeline", nullptr, false, !IsReadOnly())) {
						LoadEffect(*effect, _document.GetLength(), false);
					}
					ImGui::EndPopup();
				}
				ImGui::SameLine();
				ImGui::TextDisabled("%.2fs", effect->GetLength());
				ImGui::PopID();
			}
		}
	}

	void SynthEditor::DrawEffectMenu(float start)
	{
		if (!ImGui::BeginMenu("Add effect here")) {
			return;
		}

		if (!_document.Library.empty() && ImGui::BeginMenu("Custom")) {
			for (const CustomEffect& custom : _document.Library) {
				if (ImGui::MenuItem(custom.Name.c_str())) {
					LoadClips(custom.Clips, start, false);
				}
			}
			ImGui::EndMenu();
		}

		const std::vector<SynthEffect>& effects = SynthLibrary::GetEffects();
		for (const std::string& category : SynthLibrary::GetCategories()) {
			if (!ImGui::BeginMenu(category.c_str())) {
				continue;
			}

			for (const SynthEffect& effect : effects) {
				if (effect.Category == category && ImGui::MenuItem(effect.Name.c_str())) {
					LoadEffect(effect, start, false);
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}

	void SynthEditor::LoadEffect(const SynthEffect& effect, float start, bool replace)
	{
		std::vector<TimelineClip> clips;
		clips.reserve(effect.Layers.size());
		for (const SynthLayer& layer : effect.Layers) {
			TimelineClip clip;
			clip.Voice = layer.Voice;
			clip.Start = layer.Start;
			clips.push_back(clip);
		}
		LoadClips(clips, start, replace);
	}

	void SynthEditor::LoadClips(const std::vector<TimelineClip>& clips, float start, bool replace)
	{
		if (IsReadOnly()) {
			return;
		}

		if (clips.empty()) {
			return;
		}

		PushUndo();
		StopTimeline();

		if (replace) {
			_document.Clips.clear();
			start = 0.0f;
		}

		const int first = static_cast<int>(_document.Clips.size());
		for (const TimelineClip& source : clips) {
			TimelineClip clip = source;
			clip.Start = std::max(0.0f, start + source.Start);
			_document.Clips.push_back(clip);
		}

		Select(first);
		if (replace) {
			_playTimelineRequested = true;
		} else {
			_playClipRequested = _audition;
		}
	}

	void SynthEditor::SaveToLibrary(const std::string& name)
	{
		if (name.empty() || _document.Clips.empty()) {
			return;
		}

		float earliest = _document.Clips.front().Start;
		for (const TimelineClip& clip : _document.Clips) {
			earliest = std::min(earliest, clip.Start);
		}

		CustomEffect effect;
		effect.Name = name;
		effect.Clips = _document.Clips;
		for (TimelineClip& clip : effect.Clips) {
			clip.Start -= earliest;
		}

		PushUndo();
		_viewTemplate = nullptr;
		for (std::size_t i = 0; i < _document.Library.size(); i++) {
			if (_document.Library[i].Name == name) {
				_document.Library[i] = std::move(effect);
				_document.Active = static_cast<int>(i);
				_status = std::format("Updated {} in the library.", name);
				return;
			}
		}

		_document.Library.push_back(std::move(effect));
		_document.Active = static_cast<int>(_document.Library.size()) - 1;
		_status = std::format("Added {} to the library.", name);
	}

	void SynthEditor::UpdateLibraryEffect(std::size_t index)
	{
		if (index >= _document.Library.size() || _document.Clips.empty()) {
			return;
		}

		const std::string name = _document.Library[index].Name;
		SaveToLibrary(name);
	}

	void SynthEditor::ExportEffect(std::size_t index, const std::filesystem::path& path)
	{
		if (index >= _document.Library.size()) {
			return;
		}

		std::vector<SynthLayer> layers;
		for (const TimelineClip& clip : _document.Library[index].Clips) {
			layers.push_back({ clip.Voice, clip.Start });
		}

		const std::vector<std::int16_t> samples = Synth::Mix(layers, AudioEngine::SAMPLE_RATE);
		if (WavFile::Write(path, samples, AudioEngine::SAMPLE_RATE)) {
			_status = std::format("Exported {}.", path.string());
		} else {
			_status = std::format("Failed to export {}.", path.string());
		}
	}

	void SynthEditor::RequestSwap(const SynthEffect* effect, int custom, bool duplicate)
	{
		if (!effect && custom >= 0 && custom == _document.Active && !IsReadOnly()) {
			_playTimelineRequested = true;
			return;
		}

		if (effect && !duplicate && effect == _viewTemplate) {
			_playTimelineRequested = true;
			return;
		}

		if (HasUnboundWork()) {
			_swapTemplate = effect;
			_swapCustom = custom;
			_swapDuplicate = duplicate;
			_showSwapPopup = true;
			return;
		}
		PerformSwap(effect, custom, duplicate);
	}

	void SynthEditor::PerformSwap(const SynthEffect* effect, int custom, bool duplicate)
	{
		PushUndo();
		StopTimeline();

		if (effect && !duplicate) {
			_document.Active = -1;
			_document.Clips.clear();
			for (const SynthLayer& layer : effect->Layers) {
				TimelineClip clip;
				clip.Voice = layer.Voice;
				clip.Start = layer.Start;
				_document.Clips.push_back(clip);
			}
			_viewTemplate = effect;
			Select(_document.Clips.empty() ? -1 : 0);
			_playTimelineRequested = true;
			return;
		}

		int active = custom;
		if (effect) {
			CustomEffect created;
			created.Name = MakeUniqueName(effect->Name);
			for (const SynthLayer& layer : effect->Layers) {
				TimelineClip clip;
				clip.Voice = layer.Voice;
				clip.Start = layer.Start;
				created.Clips.push_back(clip);
			}
			_document.Library.push_back(std::move(created));
			active = static_cast<int>(_document.Library.size()) - 1;
		}

		if (active < 0 || active >= static_cast<int>(_document.Library.size())) {
			return;
		}

		_viewTemplate = nullptr;
		_document.Active = active;
		_document.Clips = _document.Library[static_cast<std::size_t>(active)].Clips;
		Select(_document.Clips.empty() ? -1 : 0);
		_playTimelineRequested = true;
	}

	void SynthEditor::DrawSwapPopup()
	{
		if (_showSwapPopup) {
			ImGui::OpenPopup("Replace current sound?");
			_showSwapPopup = false;
		}

		if (!ImGui::BeginPopupModal("Replace current sound?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			return;
		}

		ImGui::Text("The timeline is not in your sounds yet.");
		ImGui::Text("Replacing it will lose this sound.");
		ImGui::Spacing();

		if (ImGui::Button("Save as a sound first")) {
			const std::string name = _effectName[0] != '\0' ? std::string(_effectName.data()) : MakeUniqueName("Untitled");
			SaveToLibrary(name);
			PerformSwap(_swapTemplate, _swapCustom, _swapDuplicate);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Replace")) {
			PerformSwap(_swapTemplate, _swapCustom, _swapDuplicate);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	void SynthEditor::SyncActive()
	{
		if (_document.Active < 0 || _document.Active >= static_cast<int>(_document.Library.size())) {
			_document.Active = -1;
			return;
		}

		if (_document.Clips.empty()) {
			return;
		}

		float earliest = _document.Clips.front().Start;
		for (const TimelineClip& clip : _document.Clips) {
			earliest = std::min(earliest, clip.Start);
		}

		std::vector<TimelineClip> clips = _document.Clips;
		for (TimelineClip& clip : clips) {
			clip.Start -= earliest;
		}
		_document.Library[static_cast<std::size_t>(_document.Active)].Clips = std::move(clips);
	}

	bool SynthEditor::HasUnboundWork() const
	{
		if (_viewTemplate || _document.Active >= 0 || _document.Clips.empty()) {
			return false;
		}

		SynthDocument probe;
		probe.Clips = _document.Clips;
		probe.Repeat = _document.Repeat;
		return SynthProject::Serialize(probe) != _pristineText;
	}

	std::string SynthEditor::MakeUniqueName(const std::string& base) const
	{
		const auto exists = [this](const std::string& name)
		{
			return std::any_of(_document.Library.begin(), _document.Library.end(), [&name](const CustomEffect& effect)
			{
				return effect.Name == name;
			});
		};

		if (!exists(base)) {
			return base;
		}

		for (int number = 2;; number++) {
			const std::string candidate = std::format("{} {}", base, number);
			if (!exists(candidate)) {
				return candidate;
			}
		}
	}

	bool SynthEditor::IsChecked(const std::string& name) const
	{
		return std::find(_checkedSounds.begin(), _checkedSounds.end(), name) != _checkedSounds.end();
	}

	void SynthEditor::SetChecked(const std::string& name, bool checked)
	{
		const auto found = std::find(_checkedSounds.begin(), _checkedSounds.end(), name);
		if (checked && found == _checkedSounds.end()) {
			_checkedSounds.push_back(name);
		} else if (!checked && found != _checkedSounds.end()) {
			_checkedSounds.erase(found);
		}
	}

	void SynthEditor::ExportSelectedZip(const std::filesystem::path& path)
	{
		std::vector<ZipEntry> entries;
		std::vector<std::string> used;
		for (const CustomEffect& effect : _document.Library) {
			if (!IsChecked(effect.Name)) {
				continue;
			}

			std::vector<SynthLayer> layers;
			for (const TimelineClip& clip : effect.Clips) {
				layers.push_back({ clip.Voice, clip.Start });
			}

			const std::string baseName = LibraryExporter::ToFileName(effect.Name);
			std::string candidate = baseName;
			for (int number = 2; std::find(used.begin(), used.end(), candidate) != used.end(); number++) {
				candidate = std::format("{}-{}", baseName, number);
			}
			used.push_back(candidate);

			const std::vector<std::int16_t> samples = Synth::Mix(layers, AudioEngine::SAMPLE_RATE);
			entries.push_back({ candidate + ".wav", WavFile::Encode(samples, AudioEngine::SAMPLE_RATE) });
		}

		if (entries.empty()) {
			_status = "Select at least one sound.";
			return;
		}

		if (ZipFile::Write(path, entries)) {
			_status = std::format("Exported {} sounds to {}.", entries.size(), path.string());
		} else {
			_status = std::format("Failed to export {}.", path.string());
		}
	}

	void SynthEditor::ExportProjectZip(const std::filesystem::path& path)
	{
		const std::string root = LibraryExporter::ToFileName(path.stem().string());
		const std::vector<ZipEntry> entries = ProjectExporter::BuildEntries(_document, root);
		if (ZipFile::Write(path, entries)) {
			_status = std::format("Exported the project ({} sounds, {} templates) to {}.", _document.Library.size(), SynthLibrary::GetEffects().size(), path.string());
		} else {
			_status = std::format("Failed to export {}.", path.string());
		}
	}

	void SynthEditor::ExportProjectFolder(const std::filesystem::path& folder)
	{
		const std::string root = _projectPath.empty() ? std::string("project") : LibraryExporter::ToFileName(_projectPath.stem().string());
		const std::vector<ZipEntry> entries = ProjectExporter::BuildEntries(_document, root);

		for (const ZipEntry& entry : entries) {
			const std::filesystem::path target = folder / entry.Name;
			std::error_code error;
			std::filesystem::create_directories(target.parent_path(), error);

			std::ofstream stream(target, std::ios::binary);
			if (!stream) {
				_status = std::format("Failed to write {}.", target.string());
				return;
			}
			stream.write(reinterpret_cast<const char*>(entry.Data.data()), static_cast<std::streamsize>(entry.Data.size()));
		}
		_status = std::format("Exported the project to {}.", (folder / root).string());
	}

	void SynthEditor::Track()
	{
		if (ImGui::IsItemActivated()) {
			_pending = _frameStart;
		}
		if (ImGui::IsItemEdited()) {
			_dirty = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			_history.Push(_pending);
			_playClipRequested = _audition;
		}
	}

	void SynthEditor::PushUndo()
	{
		_history.Push(_frameStart);
	}

	void SynthEditor::Undo()
	{
		if (_history.Undo(_document)) {
			_viewTemplate = nullptr;
			_dirty = true;
		}
	}

	void SynthEditor::Redo()
	{
		if (_history.Redo(_document)) {
			_viewTemplate = nullptr;
			_dirty = true;
		}
	}

	float SynthEditor::SnapStart(float start, int index) const
	{
		if (ImGui::GetIO().KeyAlt) {
			return std::max(0.0f, start);
		}

		const float step = SNAP_STEPS[static_cast<std::size_t>(_snapIndex)];
		float snapped = step > 0.0f ? std::round(start / step) * step : start;

		if (_snapToClips) {
			const float length = index >= 0 ? _document.Clips[static_cast<std::size_t>(index)].GetLength() : 0.0f;
			const float threshold = SNAP_PIXELS / _pixelsPerSecond;
			float best = threshold;
			float target = snapped;
			for (int i = 0; i < static_cast<int>(_document.Clips.size()); i++) {
				if (i == index) {
					continue;
				}

				const TimelineClip& other = _document.Clips[static_cast<std::size_t>(i)];
				const std::array<float, 2> edges = { other.Start, other.Start + other.GetLength() };
				for (const float edge : edges) {
					const float startDistance = std::abs(start - edge);
					if (startDistance < best) {
						best = startDistance;
						target = edge;
					}

					const float endDistance = std::abs(start + length - edge);
					if (endDistance < best) {
						best = endDistance;
						target = edge - length;
					}
				}
			}
			snapped = target;
		}
		return std::max(0.0f, snapped);
	}

	void SynthEditor::Select(int index)
	{
		_document.Selected = index;
		_dirty = true;
	}

	void SynthEditor::AddClip(SynthPreset preset, float start)
	{
		if (IsReadOnly()) {
			return;
		}

		PushUndo();

		TimelineClip clip;
		clip.Voice = Synth::GetPreset(preset);
		clip.Start = std::max(0.0f, start);
		_document.Clips.push_back(clip);
		Select(static_cast<int>(_document.Clips.size()) - 1);
		_playClipRequested = _audition;
	}

	void SynthEditor::DuplicateClip()
	{
		if (IsReadOnly()) {
			return;
		}

		const TimelineClip* selected = GetSelectedClip();
		if (!selected) {
			return;
		}

		PushUndo();

		TimelineClip clip = *selected;
		clip.Start += clip.GetLength();
		_document.Clips.push_back(clip);
		Select(static_cast<int>(_document.Clips.size()) - 1);
	}

	void SynthEditor::DeleteClip()
	{
		if (IsReadOnly()) {
			return;
		}

		if (!GetSelectedClip()) {
			return;
		}

		PushUndo();

		_document.Clips.erase(_document.Clips.begin() + _document.Selected);
		Select(std::min(_document.Selected, static_cast<int>(_document.Clips.size()) - 1));
	}

	TimelineClip* SynthEditor::GetSelectedClip()
	{
		if (_document.Selected < 0 || _document.Selected >= static_cast<int>(_document.Clips.size())) {
			return nullptr;
		}
		return &_document.Clips[static_cast<std::size_t>(_document.Selected)];
	}

	void SynthEditor::RefreshPreview()
	{
		_dirty = false;

		const TimelineClip* clip = GetSelectedClip();
		if (!clip) {
			_preview.clear();
			return;
		}
		_preview = Synth::Render(clip->Voice, AudioEngine::SAMPLE_RATE);
	}

	void SynthEditor::PlayClip()
	{
		const TimelineClip* clip = GetSelectedClip();
		if (!clip) {
			return;
		}

		const std::shared_ptr<AudioBuffer> buffer = _audio.CreateBuffer(_preview, AudioEngine::SAMPLE_RATE);
		AudioPlayProperties properties = _properties;
		properties.Position = GetClipPosition(*clip);
		_audio.Play(buffer, properties);
	}

	void SynthEditor::StartTimeline()
	{
		StopTimeline();
		BuildSchedule();
	}

	void SynthEditor::BuildSchedule()
	{
		_schedule.clear();

		_schedule.reserve(_document.Clips.size());
		for (const TimelineClip& clip : _document.Clips) {
			const std::vector<std::int16_t> samples = Synth::Render(clip.Voice, AudioEngine::SAMPLE_RATE);
			const ScheduledSound sound = {
				.Buffer = _audio.CreateBuffer(samples, AudioEngine::SAMPLE_RATE),
				.Start = clip.Start,
				.Position = GetClipPosition(clip)
			};
			_schedule.push_back(sound);
		}

		_playhead = 0.0f;
		_playing = true;
	}

	void SynthEditor::StopTimeline()
	{
		_audio.StopAll();
		_schedule.clear();
		_playing = false;
		_playhead = 0.0f;
	}
}
