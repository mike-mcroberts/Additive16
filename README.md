# Additive 16 — VST3 synthesizer

A 16-oscillator additive synthesizer for REAPER and other VST3 hosts, built with JUCE 8.

## Requirements
- Windows 10/11, macOS, or Linux
- CMake 3.22 or later
- Git (CMake fetches JUCE 8.0.10)
- C++17 compiler (Windows: Visual Studio 2022 with Desktop development with C++)
- Internet connection on first configure to fetch JUCE

## Build on Windows (PowerShell)
From the extracted project folder:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target Additive16_VST3
```

The plugin is generated under `build/Additive16_artefacts/Release/VST3/` (path can vary with generator). Copy the **Additive 16.vst3** bundle/folder into:

`C:\Program Files\Common Files\VST3\`

Restart REAPER, go to Options > Preferences > Plug-ins > VST, then Re-scan. Add a new track, click FX, and search for **Additive 16**. Record-arm the track and enable MIDI input, or insert a MIDI item.

## Sound design
- Each of 16 partials is a sine oscillator with an independent **Level**, **Attack**, **Decay**, **Sustain**, and **Release**.
- Each partial has two pitch modes:
  - **Ratio**: 0.01–64× the played MIDI note frequency. A ratio of 1 is the fundamental, 2 is the octave, 3 is the third harmonic, etc. Fractional ratios produce inharmonic spectra.
  - **Absolute Hz**: 20–20,000 Hz regardless of played MIDI note. This permits independent oscillator tuning; a MIDI note still gates its envelope.
- The top grid chooses which partial to edit. Click or double-click a numeric control to enter precise values. The grid displays each partial's level and tuning.
- Master adjusts output level. The engine uses 12 voices of polyphony.
- Host automation and REAPER project recall work through JUCE parameter state.

## Notes / limitations
- Source project provided; this environment has not compiled or tested the VST3 binary. Build locally with the instructions above.
- Frequencies at or above 49% of sample rate are muted to reduce aliasing. Abrupt parameter changes may click; future versions could add smoothing.
- The initial pitch ratio for partial N is N, but every partial can be retuned independently.
- The envelope release slope is captured when a note-off occurs. Changes to release during an ongoing release do not affect that release.
- MIDI pitch bend, modulation, filters, presets browser, and oversampling are not implemented in v1.
