# MB-11

A fat analog-style bass drum as a plugin – from a short, clicky techno kick to a long, booming
sub tone. You play it like a little hardware box: one display, a few buttons and two value knobs.
Built with JUCE as AU, VST3, CLAP and Standalone for macOS, Windows and Linux.

- One knob turn from punchy to booming: Tune (the "punch" of the pitch sweep), Pitch, Decay,
  Harmonics for a harder, timpani-like tone
- Attack section with click and noise, including metallic noise patterns, plus an EQ to darken it
- Feels like a real device: every instance has slightly different component tolerances and drifts a
  little, just like hardware
- 100 factory sounds and 10 slots for your own, shared by all instances and formats
- Play it as a bass synth over the keyboard (C1–F3, in semitones)
- LFO with 8 waveforms, free, synced to your song's tempo or as a one-shot
- Trigger it from an audio track (X Trig side-chain input, not in the CLAP version)
- Every sound parameter can be controlled by MIDI CC and automated in your DAW
- Fine adjustment: hold Shift while dragging or scrolling

**Manual:** [English](docs/Manual.md) · [Deutsch](docs/Handbuch.md) · **Schnellstart (German quick start):** [docs/Schnellstart.md](docs/Schnellstart.md) · [PDF](docs/Schnellstart.pdf)

![MB-11 front panel](docs/images/panel.png)

## Download

Released versions are under **Releases**: AU (macOS), VST3, CLAP and a Standalone app for macOS,
Windows and Linux.
Installation is explained in the manual ([English](docs/Manual.md#1-installation) ·
[Deutsch](docs/Handbuch.md#1-installation)). A click on the MB-11 logo shows which version you have.

**macOS:** the plugins are not notarized by Apple yet, so macOS may block them. The manual explains
how to unblock any downloaded plugin – and MB-11 in one go
([English](docs/Manual.md#unblocking-plugins-on-macos) · [Deutsch](docs/Handbuch.md#plugins-auf-macos-freigeben)).

## Credits

Idea, direction, ears and the "nah, that sweep is too fast" calls: [sscheppert-blip](https://github.com/sscheppert-blip).
Every line of code, DSP and documentation was typed by AI: [Claude Code](https://claude.com/claude-code)
running **Claude Opus 5.5**, with a helping hand from [OpenAI Codex](https://openai.com/codex). So if you
like it, send a good part of the flowers to the robots – the human only pointed and said "make it go
boom". 🤖💐

## License

Public domain ([Unlicense](LICENSE)) – do whatever you like with it, no credit needed.
The plugins are built with [JUCE](https://juce.com), which has its own license terms.

<sub>Building from source: [docs/Development.md](docs/Development.md)</sub>
