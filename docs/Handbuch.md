# MB-11 – Handbuch

*[English version](Manual.md) · Kurzfassung zum Ausdrucken: [Schnellstart](Schnellstart.md) ([PDF](Schnellstart.pdf))*

![Die Frontplatte von MB-11](images/panel.png)

MB-11 ist eine analog klingende Bassdrum als Plugin – vom kurzen, klickenden Techno-Kick bis zum
langen, dröhnenden Sub-Ton. Bedient wird sie wie ein kleines Hardware-Gerät: ein Display, ein paar
Taster, zwei VALUE-Regler. Wie echte Hardware hat jede Instanz leicht andere Bauteil-Toleranzen und
driftet ein wenig.

---

## Inhalt

1. [Installation](#1-installation)
2. [Erste Schritte](#2-erste-schritte)
3. [Die Bedienelemente](#3-die-bedienelemente)
4. [Die drei Ebenen](#4-die-drei-ebenen)
5. [Die Klangparameter](#5-die-klangparameter)
6. [Master-Ebene](#6-master-ebene)
7. [2nd Function](#7-2nd-function)
8. [Häufige Aufgaben](#8-häufige-aufgaben)
9. [Presets und Speicher](#9-presets-und-speicher)
10. [MIDI](#10-midi)
11. [Tipps](#11-tipps)

---

## 1. Installation

MB-11 gibt es als **AU** (nur macOS), **VST3** und **CLAP** für macOS, Windows und Linux. Fertige
Pakete liegen bei den [Releases](https://github.com/sscheppert-blip/MB-11/releases). Eine
Standalone-App, die ohne DAW läuft, entsteht derzeit nur beim Selberbauen.

| System | Ordner |
|---|---|
| macOS AU | `~/Library/Audio/Plug-Ins/Components/` |
| macOS VST3 | `~/Library/Audio/Plug-Ins/VST3/` |
| macOS CLAP | `~/Library/Audio/Plug-Ins/CLAP/` |
| Windows VST3 | `C:\Program Files\Common Files\VST3\` |
| Windows CLAP | `C:\Program Files\Common Files\CLAP\` |
| Linux VST3 / CLAP | `~/.vst3/` / `~/.clap/` |

Danach die DAW nach neuen Plugins suchen lassen. MB-11 erscheint als Instrument des Herstellers
**sscheppert-blip**. Ein Klick aufs
MB-11-Logo zeigt, welche Version du hast. Beim Aktualisieren das alte Plugin vorher löschen.

Die CLAP-Version hat keinen X-Trig-Eingang (Auslösen per Audio).

### Plugins auf macOS freigeben

Jede heruntergeladene Datei bekommt von macOS eine versteckte „Quarantäne“-Markierung. Bei Plugins,
die nicht von Apple notarisiert sind – wie MB-11 und viele kostenlose oder Open-Source-Plugins –
kann das bedeuten:

- eine Meldung, das Plugin „kann nicht geöffnet werden“ oder „ist beschädigt“,
- oder die DAW überspringt es beim Scannen einfach.

**Der allgemeine Weg – klappt für jedes Plugin:** die Markierung einmal im **Terminal** entfernen
(Programme → Dienstprogramme → Terminal), danach die DAW neu starten:

```bash
xattr -dr com.apple.quarantine "/Pfad/zum/Plugin"
```

Tipp: `xattr -dr com.apple.quarantine ` (mit dem Leerzeichen am Ende) tippen, die Plugin-Datei aus dem
Finder ins Terminal-Fenster ziehen – der Pfad wird eingefügt – und Return drücken. `-d` löscht die
Markierung, `-r` schließt alles im Plugin-Paket ein. `xattr -l "/Pfad/zum/Plugin"` zeigt, ob noch eine
Markierung da ist.

**Für MB-11 – alle Formate auf einmal:**

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/MB-11.component ~/Library/Audio/Plug-Ins/VST3/MB-11.vst3 ~/Library/Audio/Plug-Ins/CLAP/MB-11.clap
```

Hast du nur einen Teil der Formate installiert, meldet der Befehl die fehlenden – das ist in Ordnung,
die anderen sind trotzdem freigegeben.

Mach das nur bei Plugins aus Quellen, denen du vertraust: Die Markierung ist die Frage von macOS
„Bist du sicher?“.

---

## 2. Erste Schritte

1. MB-11 auf eine Instrumentenspur legen.
2. **PLAY** drücken oder die Note **C1** spielen – die Bassdrum erklingt.
3. Den **VALUE-Encoder** (unten) drehen: Das Display zeigt die Presets `r00`–`r99`
   (Werkssounds) und `Pr0`–`Pr9` (eigene). Jeder Wechsel lädt sofort.
4. Mit **DOWN** zu einem Klangparameter blättern (die LED rechts zeigt welcher) und den Encoder drehen.

Das DAW-Projekt speichert den aktuellen Sound immer mit, auch ohne Store.

---

## 3. Die Bedienelemente

| Element | Funktion |
|---|---|
| **Display** | zeigt Preset oder Wert des gewählten Parameters |
| **VALUE-Encoder** (unten) | Wert ändern (ziehen oder scrollen). **Klick** = Load/Enter |
| **VALUE-Poti** (oben) | schnell über große Bereiche. Muss den Wert erst „abholen“, dann wird der Zeiger rot |
| **UP / DOWN** | durch die Parameter blättern |
| **PLAY** | Sound auslösen |
| **LEDs rechts** | welcher Klangparameter gewählt ist |
| **LEDs links** | welcher Master- bzw. 2nd-Function-Parameter gewählt ist |
| **Trig / Xtrig / Midi** | blinken bei Auslösung, Audio-Auslösung bzw. MIDI-Empfang |
| **MB-11-Logo** | Klick zeigt Version und Build |

**Feinjustage:** **Shift** gedrückt halten beim Ziehen oder Scrollen – der Encoder reagiert dann
3×, das Poti 4× feiner.

Am Gerät drückt man für manche Funktionen zwei Taster gleichzeitig. Mit der Maus geht das nicht,
deshalb gilt **Shift-Klick** als „zweiter Taster“:

| Zwei Taster zugleich | Bei MB-11 | Öffnet |
|---|---|---|
| UP + DOWN | **Shift-Klick auf UP oder DOWN** | Master-Ebene |
| UP + PLAY | **Shift-Klick auf PLAY** | 2nd Function |

**Tastatur** (wenn das Fenster den Fokus hat, z. B. in der Standalone-App):

| Taste | Funktion |
|---|---|
| Space | PLAY |
| ↑ / ↓ | UP / DOWN |
| ← / → | Encoder −1 / +1 (Shift: ±10) |
| Return | Encoder-Klick |
| Shift + ↑/↓ | Master-Ebene |
| Shift + Space | 2nd Function |

---

## 4. Die drei Ebenen

### Preset-Modus – keine LED leuchtet

Nach dem Start bist du hier. Das Display zeigt `Pr0`–`Pr9` (eigene Presets) oder `r00`–`r99`
(Werkssounds). **Encoder drehen** = Preset wechseln.

### Klangparameter – LED rechts

- **LED leuchtet** → der Parameter **rechts** daneben ist aktiv (Tune … EQ)
- **LED blinkt** → der **rote** Parameter **links** daneben ist aktiv (Volume, Gate, Compr, MetNze)

```
Preset ──DOWN──► Tune ► Pitch ► Decay ► Harmonics ► Pulse ► Noise ► Attack ► EQ
                                                                              │
Preset ◄──DOWN── MetNze ◄ Compr ◄ Gate ◄ Volume ◄────────────────────────────┘
                 (rot, LED blinkt)
```

**UP** läuft dieselbe Runde rückwärts. Vom Preset-Modus aus bist du mit **2× UP** bei Compr, mit
**1× UP** bei MetNze. **Zurück in den Preset-Modus:** UP über *Tune* hinaus oder DOWN über
*MetNze* hinaus.

Ein **Punkt rechts unten im Display** zeigt: Das Preset ist verändert (Edit-On). **Encoder-Klick**
verwirft die Änderungen und lädt das Preset neu.

### Master & 2nd Function – LED links

- **Master** (Shift-Klick UP/DOWN): LED leuchtet, Parameter links (Midi Ch … Store)
- **2nd Function** (Shift-Klick PLAY): LED blinkt, Parameter daneben (X Trig Level … LFO One Shot)

Mit UP/DOWN blätterst du, mit dem Encoder änderst du den Wert. **Zurück:** derselbe Shift-Klick
noch einmal oder **10 Sekunden nichts tun**.

---

## 5. Die Klangparameter

Alle Klangparameter gehen von **0 bis 255**, wie am Gerät.

| Parameter | Bewirkt | Achtung |
|---|---|---|
| **Tune** | Stärke des Pitch-Sweeps („Punch“) | nicht die Tonhöhe! |
| **Pitch** | Grundtonhöhe (10 Hz … hoch) | |
| **Decay** | Länge der Kick | |
| **Harmonics** | Sinus → härter, paukenartig | |
| **Pulse** | Klick-Anteil im Attack | nur hörbar mit Attack > 0 |
| **Noise** | Rausch-Anteil im Attack | nur hörbar mit Attack > 0 und niedrigem EQ |
| **Attack** | Lautstärke von Pulse + Noise | 0 = kein Attack |
| **EQ** | Filter, macht dumpfer | 0 = offen, höher = dumpfer |
| **Volume** | Gesamtlautstärke | am besten auf 255 lassen |
| **Gate** | Länge des Auslöseimpulses: kurz = Klick, lang = Klatschen | |
| **Compr** | kürzt den Ausklang | höher = kürzer, kein Kompressor |
| **MetNze** | 0 = Rauschen, sonst metallische Muster | jeder Wert klingt anders – ausprobieren! |

---

## 6. Master-Ebene

Shift-Klick auf UP oder DOWN. Die Einstellungen gehören zum Preset bzw. zur Instanz.

| Parameter | Werte | Bewirkt |
|---|---|---|
| **Midi Ch** | 1–16 | MIDI-Kanal, auf dem MB-11 hört (Standard 1) |
| **Split Mode** | `SM1`, `SM2` | `SM1`: nur C1 löst aus. `SM2`: die Noten C1–F3 lösen mit Tonhöhe aus (Bass-Synth) |
| **Pitch Mode** | `Lin`, `SEM` | wie die Tonhöhe in `SM2` den Noten folgt: `SEM` in Halbtönen, `Lin` linear (jede Note hebt die Frequenz um denselben Betrag) |
| **LFO Wave** | siehe unten | Wellenform des LFO |
| **LFO Speed** | `Mid`, 40–290 | Tempo in BPM, ein LFO-Durchlauf pro Viertelnote. `Mid` folgt dem Tempo der DAW |
| **LFO Int** | 0–255 | wie stark der LFO die Tonhöhe bewegt (0 = aus) |
| **LFO Sync** | `FrE`, `SYn` | `FrE`: läuft frei. `SYn`: startet bei jedem Schlag neu |
| **Store** | `Sto` | speichert den Sound (siehe [Häufige Aufgaben](#8-häufige-aufgaben)) |

**LFO-Wellenformen:** `SuP` Sägezahn steigend, `Sdo` Sägezahn fallend, `Sin` Sinus, `Si-` Sinus
umgekehrt, `tri` Dreieck, `tr-` Dreieck umgekehrt, `rCt` Rechteck, `rC-` Rechteck umgekehrt.

---

## 7. 2nd Function

Shift-Klick auf PLAY.

| Parameter | Werte | Bewirkt |
|---|---|---|
| **X Trig Level** | 0–255 | Empfindlichkeit der Audio-Auslösung: höher = empfindlicher |
| **X Trig On/Off** | `oFF`, `_on` | Auslösen per Audio am Sidechain-Eingang „X Trig In“ |
| **Sys Dmp** | `dMP` | Encoder-Klick speichert das aktuelle Preset als `.syx`-Datei |
| **Sys Load** | `_Ld` | Encoder-Klick lädt eine `.syx`-Datei ins aktuelle Preset |
| **LFO One Shot** | `oFF`, `onE` | `onE`: der LFO läuft pro Schlag nur einen Durchgang |

---

## 8. Häufige Aufgaben

**Sound auswählen**
Im Preset-Modus den Encoder drehen und PLAY drücken.

**Sound anpassen**
DOWN bis zum gewünschten Parameter, dann den Encoder drehen.

**Änderungen verwerfen**
**Encoder klicken** – das Preset wird neu geladen.

**Sound speichern**
1. Shift-Klick UP → Master-Ebene
2. DOWN bis **Store** (letzte LED links), das Display zeigt `Sto`
3. Encoder klicken → das Display blinkt
4. Speicherplatz mit dem Encoder wählen (`Pr0`–`Pr9` empfohlen)
5. Encoder klicken → gespeichert

**Melodisch spielen (Bass-Synth)**
Master-Ebene → **Split Mode** auf `SM2`, **Pitch Mode** auf `SEM`. Jetzt spielen die Noten C1–F3 in
Halbtönen. Tipp: Tune niedrig, Decay lang.

**LFO nutzen**
Master-Ebene → **LFO Int** hochdrehen (0 = aus), dazu **LFO Wave** und **LFO Speed** einstellen.
`Mid` als Speed läuft synchron zum Tempo der DAW. **LFO Sync** `SYn` startet den LFO bei jedem
Schlag neu.

**Per Audio auslösen (X Trig)**
In der DAW ein Signal auf den Sidechain-Eingang „X Trig In“ legen. Dann 2nd Function →
**X Trig On/Off** auf `_on` und **X Trig Level** so weit erhöhen, bis es sauber auslöst. MB-11 gibt
dabei auch eine MIDI-Note C1 aus – so lässt sich eine Audiospur in MIDI-Trigger verwandeln. (Nicht in
der CLAP-Version.)

**Preset als Datei sichern / laden**
2nd Function → **Sys Dmp** (Encoder-Klick speichert eine `.syx`-Datei) bzw. **Sys Load**
(Encoder-Klick lädt eine Datei ins aktuelle Preset).

---

## 9. Presets und Speicher

Alle 110 Speicherplätze (`Pr0`–`Pr9`, `r00`–`r99`) stehen in **einer Datei**, die sich alle
MB-11-Instanzen und Formate teilen – wie der Speicher im Gerät:

| System | Datei |
|---|---|
| macOS | `~/Library/Application Support/MB-11/PresetBank.xml` |
| Windows | `%APPDATA%\MB-11\PresetBank.xml` |
| Linux | `~/.config/MB-11/PresetBank.xml` |

- Die Datei entsteht beim ersten **Store**. Vorher gelten die Werkssounds.
- **Sichern:** Datei kopieren. **Werkszustand:** Datei löschen.
- Du musst nichts selbst dort ablegen – MB-11 speichert automatisch.
- Unabhängig davon speichert dein DAW-Projekt den aktuellen Sound mit, auch ungespeicherte
  Änderungen.
- `.syx`-Dateien aus **Sys Dmp** landen dort, wo du sie im Dialog hinlegst.

---

## 10. MIDI

- MB-11 hört auf den in der Master-Ebene eingestellten **MIDI-Kanal** (Standard: 1).
- **Note C1 (36)** löst die Bassdrum aus, im Split Mode `SM2` die Noten C1–F3. Die Anschlagstärke
  bestimmt die Lautstärke.
- **Program Change** wählt das Preset: 0–9 = `Pr0`–`Pr9`, 10–109 = `r00`–`r99`.
- Alle Klangparameter sind per **MIDI-CC** steuerbar und in der DAW
  automatisierbar:

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

## 11. Tipps

- **Mehr Punch:** Tune höher, Decay kürzer, etwas Attack mit Pulse.
- **Langer Sub-Bass:** Pitch tief, Decay lang, Tune niedrig, Harmonics auf 0.
- **Härter, paukenartig:** Harmonics hochdrehen.
- **Klick statt Wumms:** Gate kurz, Attack und Pulse hoch, EQ niedrig.
- **Metallische Effekte:** Attack hoch, Noise hoch, EQ niedrig und MetNze durchprobieren.
- **Jede Instanz klingt leicht anders?** Das ist Absicht: Bauteil-Toleranzen und Drift wie bei
  echter Hardware. Der Charakter einer Instanz bleibt mit dem Projekt gespeichert.
