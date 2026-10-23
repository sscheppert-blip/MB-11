# MB-11 – Development

## Building

Needs CMake ≥ 3.22, a C++20 compiler (Xcode, Visual Studio 2022, GCC/Clang) and Ninja (optional).
JUCE and clap-juce-extensions are fetched by CMake.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/MB11Tests_artefacts/Release/MB11Tests
```

| Option | Default | |
|--------|---------|-|
| `MB11_COPY_PLUGIN` | `ON` | Copy the plugins into the user's plugin folders after building |
| `MB11_BUILD_TESTS` | `ON` | Build the offline tests |

On Linux install the JUCE dependencies first (see `.github/workflows/build.yml`).

The version popup (click on the logo) shows `VERSION` from `CMakeLists.txt` and a build number:
the commit count and hash, refreshed on every build by `cmake/BuildInfo.cmake`. A `+` after the
hash means the build had uncommitted changes.

## Tests and tools

Besides the offline tests, the test runner writes the first 32 factory sounds to a WAV file
(`--render demo.wav`), a panel screenshot (`--snapshot panel.png`, or `--snapshot-info` with the
version popup open) and measures the CPU load (`--bench`).

README image: `docs/images/panel.png` (`--snapshot …`, downscaled to 1400 px).

After editing `docs/Schnellstart.md`, regenerate the PDF with `python3 tools/make_quickstart_pdf.py`
(needs reportlab and the DejaVu fonts). `tools/check-sensitive-data.sh` (needs ripgrep) scans the
repository for credentials and local paths; the release workflow runs it too. `.githooks/pre-push` runs it before every push once enabled with `git config core.hooksPath .githooks`.

## Releases

Builds run only on demand (pushes don't trigger them, to save GitHub Actions minutes). To publish a
new version, bump `VERSION` in `CMakeLists.txt`, optionally add release notes as
`docs/releases/<version>.md` (first line `# <title>`, the rest are the notes), commit and push, then:

- **On a Mac:** `tools/release-mac.sh 0.3.0` builds, tests and validates the macOS plugins locally as
  universal binaries, creates a draft release with them, then lets GitHub build only Linux and
  Windows and publish. `--dry-run` stops after packaging.
- **On a PC:** `gh workflow run build -f version=0.3.0` (or Actions → build → Run workflow) builds
  all three platforms on GitHub.

Either way the release `v0.3.0` gets AU (macOS), VST3, CLAP and Standalone packages named
`MB-11-<version>-<system>-<format>`. Without a notes file, GitHub generates the notes from the commits.

The macOS builds are only ad-hoc signed. Gatekeeper may block them on other Macs until they are
signed and notarized.

## Validation (macOS)

Before a release, validate the installed plugins:

```bash
auval -v aumu Mb11 Sbli
pluginval --strictness-level 10 --validate-in-process --validate ~/Library/Audio/Plug-Ins/VST3/MB-11.vst3
clap-validator validate ~/Library/Audio/Plug-Ins/CLAP/MB-11.clap
```

`tools/release-mac.sh` runs them automatically when they are installed.

## Presets

All 110 preset slots are stored in one file shared by every instance and format
(`~/Library/Application Support/MB-11/PresetBank.xml`, `%APPDATA%\MB-11\PresetBank.xml`,
`~/.config/MB-11/PresetBank.xml`). The file is created on the first *Store*; deleting it returns to
the factory presets. A preset is 16 SysEx bytes in MIDI CC order plus the LFO wave.
