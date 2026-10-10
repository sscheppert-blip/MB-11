# MB-11

Analog-style bass drum synthesizer modelled on the operating concept of the Jomox MBase11.
Built with JUCE as AU, VST3, CLAP and Standalone for macOS, Windows and Linux.

- Modelled analog circuit: triangle-core VCO with shaper, harmonics morph, pitch envelope,
  capacitor envelopes, OTA VCA, coupling capacitors, component tolerances per instance (4× oversampled)
- Pulse/noise attack section with metallic noise patterns, EQ low-pass
- Hardware workflow: display, Up/Down, endless encoder, value pot with pickup, master and 2nd function pages
- 10 user and 100 factory presets, SysEx dump/load, MIDI CC map of the original
- LFO with 8 waveforms, BPM or host sync, retrigger and one-shot
- X Trig sidechain input with trigger-to-MIDI (AU, VST3, Standalone)

**Quick start (German):** [docs/Schnellstart.md](docs/Schnellstart.md)

## Downloads

Every push to `main` builds all formats for all platforms. The packages are attached to the
workflow run under **Actions → build → Artifacts**.

The macOS builds are only ad-hoc signed. Gatekeeper may block them on other Macs until they are
signed and notarized; locally you can allow them with `xattr -dr com.apple.quarantine <plugin>`.

## Presets

All 110 preset slots are stored in one file shared by every instance and format:

| System  | File |
|---------|------|
| macOS   | `~/Library/Application Support/MB-11/PresetBank.xml` |
| Windows | `%APPDATA%\MB-11\PresetBank.xml` |
| Linux   | `~/.config/MB-11/PresetBank.xml` |

The file is created on the first *Store*. Delete it to return to the factory presets.

## Building

Requirements: CMake 3.22+, a C++20 compiler (Xcode, Visual Studio 2022, GCC/Clang), Ninja recommended.
JUCE and clap-juce-extensions are fetched automatically.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/MB11Tests_artefacts/Release/MB11Tests
```

Options:

| Option | Default | |
|--------|---------|-|
| `MB11_COPY_PLUGIN` | `ON` | Copy the plugins into the user's plugin folders after building |
| `MB11_BUILD_TESTS` | `ON` | Build the offline tests |

On Linux install the JUCE dependencies first (see `.github/workflows/build.yml`).

## License

MB-11 uses [JUCE](https://juce.com), which is licensed under AGPLv3 or a commercial JUCE licence.
"Jomox" and "MBase" are trademarks of their respective owner; MB-11 is an independent project and
not affiliated with Jomox.
