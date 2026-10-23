# MB-11 – Manual

*[Deutsche Fassung](Handbuch.md) · German quick start to print: [Schnellstart](Schnellstart.md) ([PDF](Schnellstart.pdf))*

![The MB-11 front panel](images/panel.png)

MB-11 is an analog-style bass drum as a plugin – from a short, clicky techno kick to a long, booming
sub tone. You play it like a little hardware box: one display, a few buttons and two value knobs.
Like real hardware, every instance has slightly different component tolerances and drifts a little.

---

## Contents

1. [Installation](#1-installation)
2. [Getting started](#2-getting-started)
3. [The controls](#3-the-controls)
4. [The three levels](#4-the-three-levels)
5. [Sound parameters](#5-sound-parameters)
6. [Master level](#6-master-level)
7. [2nd function](#7-2nd-function)
8. [Common tasks](#8-common-tasks)
9. [Presets and memory](#9-presets-and-memory)
10. [MIDI](#10-midi)
11. [Tips](#11-tips)

---

## 1. Installation

MB-11 comes as **AU** (macOS only), **VST3** and **CLAP** for macOS, Windows and Linux. Ready-made
packages are on the [releases page](https://github.com/sscheppert-blip/MB-11/releases), together with
a **Standalone** app that runs without a DAW.

| System | Folder |
|---|---|
| macOS AU | `~/Library/Audio/Plug-Ins/Components/` |
| macOS VST3 | `~/Library/Audio/Plug-Ins/VST3/` |
| macOS CLAP | `~/Library/Audio/Plug-Ins/CLAP/` |
| Windows VST3 | `C:\Program Files\Common Files\VST3\` |
| Windows CLAP | `C:\Program Files\Common Files\CLAP\` |
| Linux VST3 / CLAP | `~/.vst3/` / `~/.clap/` |

Then let your DAW rescan its plugins. MB-11 shows up as an instrument by the manufacturer
**sscheppert-blip**. A click on the MB-11 logo shows
which version you have. When updating, delete the old plugin first.

**Standalone app:** unpack and start it from anywhere – on macOS `MB-11.app` (e.g. into
Applications), on Windows `MB-11.exe`, on Linux the `MB-11` program. Pick your audio and MIDI devices
under **Options → Audio/MIDI Settings**. It shares the preset memory with the plugins.

The CLAP version has no X Trig input (triggering from audio).

### Unblocking plugins on macOS

Every file you download gets a hidden "quarantine" mark from macOS. For plugins that are not
notarized by Apple – like MB-11 and many free or open-source plugins – this can mean:

- a message that the plugin "cannot be opened" or "is damaged",
- or your DAW silently skips it during the scan.

**The general way – works for any plugin:** remove the mark once in the **Terminal**
(Applications → Utilities → Terminal), then restart your DAW:

```bash
xattr -dr com.apple.quarantine "/path/to/the/plugin"
```

Tip: type `xattr -dr com.apple.quarantine ` (with the space at the end), drag the plugin file from the
Finder into the Terminal window – the path is filled in – and press Return. `-d` deletes the mark,
`-r` includes everything inside the plugin bundle. `xattr -l "/path/to/the/plugin"` shows whether a
mark is still there.

**For MB-11 – all formats at once:**

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/MB-11.component ~/Library/Audio/Plug-Ins/VST3/MB-11.vst3 ~/Library/Audio/Plug-Ins/CLAP/MB-11.clap /Applications/MB-11.app
```

If you only installed some of the formats, the command reports the missing ones – that's fine, the
others are unblocked anyway.

Only do this for plugins from sources you trust: the mark is macOS's way of asking "are you sure?".

---

## 2. Getting started

1. Put MB-11 on an instrument track.
2. Press **PLAY** or play the note **C1** – the bass drum sounds.
3. Turn the **VALUE encoder** (bottom): the display shows the presets `r00`–`r99` (factory sounds)
   and `Pr0`–`Pr9` (your own). Every change loads immediately.
4. Step to a sound parameter with **DOWN** (the LED on the right shows which) and turn the encoder.

Your DAW project always stores the current sound, even without Store.

---

## 3. The controls

| Control | Function |
|---|---|
| **Display** | shows the preset or the value of the selected parameter |
| **VALUE encoder** (bottom) | change the value (drag or scroll). **Click** = Load/Enter |
| **VALUE pot** (top) | fast over large ranges. It has to "pick up" the value first, then the pointer turns red |
| **UP / DOWN** | step through the parameters |
| **PLAY** | trigger the sound |
| **LEDs on the right** | which sound parameter is selected |
| **LEDs on the left** | which master or 2nd-function parameter is selected |
| **Trig / Xtrig / Midi** | flash on a trigger, an audio trigger or incoming MIDI |
| **MB-11 logo** | click to show version and build |

**Fine adjustment:** hold **Shift** while dragging or scrolling – the encoder then reacts 3×, the pot
4× finer.

On the device, some functions need two buttons pressed together. A mouse can't do that, so
**Shift-click** counts as "second button":

| Two buttons together | In MB-11 | Opens |
|---|---|---|
| UP + DOWN | **Shift-click on UP or DOWN** | master level |
| UP + PLAY | **Shift-click on PLAY** | 2nd function |

**Keyboard** (when the window has focus, e.g. in the Standalone app):

| Key | Function |
|---|---|
| Space | PLAY |
| ↑ / ↓ | UP / DOWN |
| ← / → | encoder −1 / +1 (Shift: ±10) |
| Return | encoder click |
| Shift + ↑/↓ | master level |
| Shift + Space | 2nd function |

---

## 4. The three levels

### Preset mode – no LED lit

This is where you start. The display shows `Pr0`–`Pr9` (your presets) or `r00`–`r99` (factory
sounds). **Turn the encoder** to change the preset.

### Sound parameters – LED on the right

- **LED lit** → the parameter **to its right** is active (Tune … EQ)
- **LED flashing** → the **red** parameter **to its left** is active (Volume, Gate, Compr, MetNze)

```
Preset ──DOWN──► Tune ► Pitch ► Decay ► Harmonics ► Pulse ► Noise ► Attack ► EQ
                                                                              │
Preset ◄──DOWN── MetNze ◄ Compr ◄ Gate ◄ Volume ◄────────────────────────────┘
                 (red, LED flashing)
```

**UP** goes the same round backwards. From preset mode, **2× UP** takes you to Compr, **1× UP** to
MetNze. **Back to preset mode:** UP past *Tune* or DOWN past *MetNze*.

A **dot at the bottom right of the display** shows that the preset has been changed (edit on). An
**encoder click** discards the changes and reloads the preset.

### Master & 2nd function – LED on the left

- **Master** (Shift-click UP/DOWN): LED lit, parameters on the left (Midi Ch … Store)
- **2nd function** (Shift-click PLAY): LED flashing, parameters next to it (X Trig Level … LFO One Shot)

Step with UP/DOWN, change the value with the encoder. **Back:** the same Shift-click again, or
**10 seconds without touching anything**.

---

## 5. Sound parameters

All sound parameters run from **0 to 255**, like on the device.

| Parameter | Effect | Note |
|---|---|---|
| **Tune** | depth of the pitch sweep ("punch") | not the pitch! |
| **Pitch** | base pitch (10 Hz … high) | |
| **Decay** | length of the kick | |
| **Harmonics** | sine → harder, timpani-like | |
| **Pulse** | click in the attack | only audible with Attack > 0 |
| **Noise** | noise in the attack | only audible with Attack > 0 and a low EQ |
| **Attack** | level of Pulse + Noise | 0 = no attack |
| **EQ** | filter, makes it darker | 0 = open, higher = darker |
| **Volume** | overall volume | best left at 255 |
| **Gate** | length of the trigger pulse: short = click, long = slap | |
| **Compr** | shortens the tail | higher = shorter, not a compressor |
| **MetNze** | 0 = noise, otherwise metallic patterns | every value sounds different – try them! |

---

## 6. Master level

Shift-click on UP or DOWN. These settings belong to the preset or the instance.

| Parameter | Values | Effect |
|---|---|---|
| **Midi Ch** | 1–16 | MIDI channel MB-11 listens on (default 1) |
| **Split Mode** | `SM1`, `SM2` | `SM1`: only C1 triggers. `SM2`: the notes C1–F3 trigger with pitch (bass synth) |
| **Pitch Mode** | `Lin`, `SEM` | how pitch follows the notes in `SM2`: `SEM` in semitones, `Lin` linearly (every note raises the frequency by the same amount) |
| **LFO Wave** | see below | LFO waveform |
| **LFO Speed** | `Mid`, 40–290 | tempo in BPM, one LFO cycle per quarter note. `Mid` follows the DAW tempo |
| **LFO Int** | 0–255 | how far the LFO moves the pitch (0 = off) |
| **LFO Sync** | `FrE`, `SYn` | `FrE`: runs freely. `SYn`: restarts on every hit |
| **Store** | `Sto` | stores the sound (see [Common tasks](#8-common-tasks)) |

**LFO waveforms:** `SuP` rising saw, `Sdo` falling saw, `Sin` sine, `Si-` inverted sine, `tri` triangle,
`tr-` inverted triangle, `rCt` square, `rC-` inverted square.

---

## 7. 2nd function

Shift-click on PLAY.

| Parameter | Values | Effect |
|---|---|---|
| **X Trig Level** | 0–255 | sensitivity of the audio trigger: higher = more sensitive |
| **X Trig On/Off** | `oFF`, `_on` | trigger from audio at the side-chain input "X Trig In" |
| **Sys Dmp** | `dMP` | an encoder click saves the current preset as a `.syx` file |
| **Sys Load** | `_Ld` | an encoder click loads a `.syx` file into the current preset |
| **LFO One Shot** | `oFF`, `onE` | `onE`: the LFO runs a single cycle per hit |

---

## 8. Common tasks

**Pick a sound**
In preset mode, turn the encoder and press PLAY.

**Edit a sound**
DOWN to the parameter you want, then turn the encoder.

**Discard changes**
**Click the encoder** – the preset is reloaded.

**Store a sound**
1. Shift-click UP → master level
2. DOWN to **Store** (last LED on the left), the display shows `Sto`
3. Click the encoder → the display flashes
4. Pick a slot with the encoder (`Pr0`–`Pr9` recommended)
5. Click the encoder → stored

**Play it as a bass synth**
Master level → **Split Mode** to `SM2`, **Pitch Mode** to `SEM`. The notes C1–F3 now play in
semitones. Tip: low Tune, long Decay.

**Use the LFO**
Master level → turn up **LFO Int** (0 = off), and set **LFO Wave** and **LFO Speed**. `Mid` as speed
follows the DAW tempo. **LFO Sync** `SYn` restarts the LFO on every hit.

**Trigger from audio (X Trig)**
Send a signal to the side-chain input "X Trig In" in your DAW. Then 2nd function → **X Trig On/Off**
to `_on`, and raise **X Trig Level** until it triggers cleanly. MB-11 also sends a MIDI note C1 – so
an audio track becomes MIDI triggers. (Not in the CLAP version.)

**Save / load a preset as a file**
2nd function → **Sys Dmp** (encoder click saves a `.syx` file) or **Sys Load** (encoder click loads a
file into the current preset).

---

## 9. Presets and memory

All 110 slots (`Pr0`–`Pr9`, `r00`–`r99`) live in **one file**, shared by all MB-11 instances and
formats – like the memory in the device:

| System | File |
|---|---|
| macOS | `~/Library/Application Support/MB-11/PresetBank.xml` |
| Windows | `%APPDATA%\MB-11\PresetBank.xml` |
| Linux | `~/.config/MB-11/PresetBank.xml` |

- The file is created at the first **Store**. Until then, the factory sounds apply.
- **Back up:** copy the file. **Factory state:** delete the file.
- You don't need to put anything there yourself – MB-11 saves automatically.
- Independently of that, your DAW project stores the current sound, including unsaved changes.
- `.syx` files from **Sys Dmp** go wherever you put them in the dialog.

---

## 10. MIDI

- MB-11 listens on the **MIDI channel** set on the master level (default: 1).
- **Note C1 (36)** triggers the bass drum, in split mode `SM2` the notes C1–F3. Velocity sets the
  level.
- **Program change** picks the preset: 0–9 = `Pr0`–`Pr9`, 10–109 = `r00`–`r99`.
- Every sound parameter can be controlled by **MIDI CC** and automated in your DAW:

| CC | Parameter | CC | Parameter |
|---|---|---|---|
| 100 | Tune | 113 | Compr |
| 101 | Pitch | 114 | Gate |
| 102 | Decay | 115 | MetNze |
| 103 | Harmonics | 117 | Volume |
| 104 | Pulse | 120 | LFO Sync |
| 105 | Noise | 121 | LFO Int |
| 106 | Attack | 122 | LFO Speed |
| 107 | EQ | 123 | LFO One Shot |

---

## 11. Tips

- **More punch:** higher Tune, shorter Decay, a little Attack with Pulse.
- **Long sub bass:** low Pitch, long Decay, low Tune, Harmonics at 0.
- **Harder, timpani-like:** turn up Harmonics.
- **Click instead of boom:** short Gate, high Attack and Pulse, low EQ.
- **Metallic effects:** high Attack, high Noise, low EQ, and try MetNze values.
- **Every instance sounds slightly different?** That's on purpose: component tolerances and drift like
  real hardware. The character of an instance is stored with the project.
