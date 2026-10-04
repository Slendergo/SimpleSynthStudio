#SimpleSynthStudio

A small sound effect studio.

Build sounds from synth voices on a timeline, position them in 2D space to hear their stereo placement, organize your sounds in a project library, and export them as WAV files.

Runs on Windows and Linux. Built with C++20, Dear ImGui, GLFW, and OpenAL Soft.

## Build
Requirements: CMake 3.21+, a C++20 compiler (MSVC 2022+, GCC 13+ or Clang 17+ with libstdc++), and an OpenGL driver.

```
build.bat  (Windows)
./build.sh (Linux)
```

Both take an optional configuration (`Debug` or `Release`, default `Release`) and read extra CMake options from the `CMAKE_ARGS` environment variable. By hand, the same thing is:

```
cmake -S . -B build
cmake --build build --config Release
```

On the first configure, GLFW and OpenAL Soft are downloaded and built unless they are already installed.
To use installed copies only, pass `-DSIMPLESYNTH_FETCH_DEPENDENCIES=OFF`.

Linux packages (Debian/Ubuntu): `sudo apt install build-essential cmake libglfw3-dev libopenal-dev libgl-dev xorg-dev`.

Open, Save As and the WAV exports use the system file dialogs. On Windows they are built in. On Linux the program calls `zenity` (GNOME and most distros) or `kdialog` (KDE), so install one of them: `sudo apt install zenity`.

On Windows the MSVC runtime is linked statically, so no redistributable is needed. Turn that off with `-DSIMPLESYNTH_STATIC_RUNTIME=OFF`. The executable and `OpenAL32.dll` end up together in `build/Release`.

## Usage
- **Timeline:** every clip is one synth voice. Drag clips to move them (snaps to the grid and to other clips; hold Alt to disable). right click the timeline to add a clip or an effect, right click a clip for Play / Duplicate / Delete / Replace. Tick **Repeat** to loop playback.
- **Voice tab:** waveform, pitch slide, vibrato, pulse width, low pass filter and an attack/decay/sustain/release envelope.
- **My Sounds panel:** everything you have made in this project, with a search box. *+ New sound* starts a blank one, and anything you edit while a sound is open is saved into it automatically. Click a sound to edit it; right click for edit, add to timeline, duplicate, update from timeline, rename, export as WAV or delete. Tick the boxes, then *Export > Selected sounds as ZIP* writes just those sounds. *Export > Project as ZIP* (or *to a folder*; both are also under the File menu) writes a whole project: your `.psynth` file, every sound under `sounds/`, and every built in template under `templates/<category>/`. A timeline that is not a stored sound yet shows a name box and *Save* at the top. The sounds are stored inside the project file.
- **Templates panel:** built in, readonly samples (guns, bows, melee, hits, magic, player, pickups, UI, movement, enemies, environment, alerts). Clicking one opens it readonly so you can play and inspect it; nothing is added to your sounds. Use *Duplicate to My Sounds* (the banner button or the right click menu) to get an editable copy. Opening a template replaces the timeline, with a warning if the current timeline is not stored as a sound yet.
- **Audio Placement panel (right):** drag the dot on the top down pad to hear a clip to the left, right, in front or behind you.
- **Projects:** `project.psynth` File menu: New, Open, Save, Save As.
- **Shortcuts:** Space plays the timeline, Ctrl+N / O / S / Shift+S for projects, Ctrl+Z undo, Ctrl+Y or Ctrl+Shift+Z redo, Ctrl+D duplicate, Delete removes the selected clip.

To get the built in templates as WAV files, use File > Export Project as ZIP (or to a folder); a project export always includes every template.

Open a project directly: `SimpleSynthStudio path/to/file.psynth`.

## Third Party
- Dear ImGui (MIT, bundled in `vendor/imgui`)
- GLFW (zlib)
- OpenAL Soft (LGPL, fetched at configure time and linked dynamically).
