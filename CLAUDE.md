# MB-11 – project notes for coding agents

Analog-style bass drum plugin modelled on the Jomox MBase11 (AU, VST3, CLAP, Standalone; macOS/Windows/Linux).
Built with JUCE 9.0.3 and clap-juce-extensions, both fetched by CMake. User-facing docs: `README.md`,
`docs/Schnellstart.md` (German quick start).

## Rules

- **No personal data** anywhere in code, docs, commits or bundle IDs: no real names, hostnames or local
  user paths. Git author is `sscheppert-blip` with the GitHub noreply address (repo-local config).
  `tools/check-sensitive-data.sh` (needs ripgrep) scans for credentials and local paths.
- The repository stays **private**.
- Name is **MB-11** (not "MBase"/"Jomox" – trademarks). Bundle/CLAP ID `io.github.sscheppert-blip.mb11`.
- Other projects of the user are not templates for this one; decide MB-11 matters on their own merits.
- The original's manual (`MBase11_D.pdf`) is copyrighted and gitignored – never commit it.

## Design decisions

- **Hardware workflow, not knobs:** display, Up/Down, endless encoder (click = Load/Enter), value pot
  with pickup, LED columns. Sound parameters page through Up/Down; Up+Down = master page, Up+Play =
  2nd function (Shift-click in the GUI, 10 s timeout). Logic lives in `Source/ui/FrontPanelModel.*`.
- Parameter semantics follow the manual: Tune = pitch envelope depth, Pitch = base pitch,
  EQ 0 = open, Compr higher = shorter tail, X Trig Level higher = more sensitive.
- **Analog behaviour is always on** (no switch), like the hardware: shaper VCO, OTA VCA with CV
  feedthrough, coupling caps, asymmetric output, drift, per-instance component tolerances
  (`unit` seed saved in the plugin state). Engine runs 4x (≤48 kHz) / 2x (≤96 kHz) oversampled.
- Presets: Pr0–Pr9 user + r00–r99 factory, one shared bank file
  (`~/Library/Application Support/MB-11/PresetBank.xml`, `%APPDATA%\MB-11\…`, `~/.config/MB-11/…`).
  Preset = 16 SysEx bytes in MIDI CC order + LFO wave.
- MIDI CCs set parameters via a pending value + message-thread update (never `setValueNotifyingHost`
  on the audio thread – clap-validator rejects it).
- CLAP has no X Trig sidechain: the wrapper would report it as in-place pair of the main output.

## Layout

| Path | |
|---|---|
| `Source/dsp/Engine.*` | Sound engine, JUCE-free (mapping functions, MetalNoise, Unit tolerances) |
| `Source/Presets.*` | Factory bank, bank file, SysEx |
| `Source/Parameters.*` | Parameter IDs/layout, preset field mapping |
| `Source/PluginProcessor.*` | MIDI, CCs, split/pitch mode, X Trig, oversampling, presets |
| `Source/PluginEditor.*`, `Source/ui/*` | Front panel view, widgets, operating logic |
| `tests/Tests.cpp` | Offline tests; `--render <wav>`, `--snapshot <png>`, `--bench` |

## Build & verify

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64"
cmake --build build
./build/MB11Tests_artefacts/Release/MB11Tests
```

Before calling a change done, also run the validators on the installed plugins (macOS):
`auval -v aumu Mb11 Sbli`, `pluginval --strictness-level 10 --validate-in-process --validate <vst3/component>`,
`clap-validator validate <clap>`. CI (`.github/workflows/build.yml`) runs **only on demand**
(`gh workflow run build -f version=X.Y.Z`, matching `VERSION` in `CMakeLists.txt`) to save Actions
minutes: it builds and tests AU, VST3 and CLAP on all platforms and publishes release `vX.Y.Z`.
Pushes do not trigger builds; don't re-add push/tag triggers.

## Open points

- Sound has only been verified by measurement (renders, spectra), not against recordings of a real
  MBase11. Real recordings / SysEx dumps would allow matching curves and confirming the dump byte order.
- Unknown how the original reaches the master page (assumed Up+Down).
- Ideas not done yet: bridged-T resonator for more attack "wood", velocity affecting attack/sweep,
  SysEx over MIDI, standalone pattern sequencer, macOS signing/notarisation, installers.
- Windows/Linux builds pass CI but have not been tried in a host.
