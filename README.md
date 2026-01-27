# UDS - Universal Delay System

8-band configurable delay matrix VST3 plugin inspired by the **Yamaha UD Stomp** with visual cable patching.

---

## Features

- **8 independent delay bands** (up to 700ms each)
- **Visual routing editor** with drag-and-drop cables
- **4 delay algorithms**: Digital, Analog, Tape, Lo-Fi
- **Per-band modulation**: LFO (Sine/Tri/Saw/Square), filters, ping-pong
- **8-stage audio safety** protection for equipment and hearing
- **Preset system** with save/load functionality

---

## Audio Safety

UDS includes comprehensive audio protection:

| Protection | Purpose |
|------------|---------|
| Extreme peak detection | Instant mute at +12dB |
| DC offset blocking | Prevents woofer damage |
| Soft-knee limiting | Catches feedback runaway |
| Slew rate limiting | Prevents ultrasonic tweeter damage |
| Hard clipping | Final safety net |

---

## Build

### Requirements

- CMake 3.22+
- MSVC 2022 (Windows) / Xcode (macOS)
- No external dependencies (JUCE fetched via CPM)

### Steps

```bash
# Configure
cmake -B build -S .

# Build Release
cmake --build build --config Release

# Output locations:
# VST3: build/UDS_artefacts/Release/VST3/
# Standalone: build/UDS_artefacts/Release/Standalone/
```

### Run Tests (optional)

```bash
# Install Catch2 first
vcpkg install catch2

# Build with tests
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release --target UDS_Tests
./build/Tests/Release/UDS_Tests.exe
```

---

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+Z | Undo routing change |
| Ctrl+Shift+Z | Redo routing change |
| Double-click slider | Reset to default |

---

## Documentation

| Document | Purpose |
|----------|---------|
| [Roadmap.md](Roadmap.md) | Phased development plan |
| [State.md](State.md) | Current project state |
| [Architecture.md](Architecture.md) | System design & components |
| [CHANGELOG.md](CHANGELOG.md) | Version history |

---

## Project Structure

```
UDS/
├── Source/
│   ├── Core/           # DSP: DelayBandNode, DelayMatrix, SafetyLimiter
│   ├── UI/             # JUCE components: NodeEditorCanvas, BandNodeComponent
│   ├── PluginProcessor.h/cpp
│   └── PluginEditor.h/cpp
├── Tests/              # Catch2 unit tests
├── .github/workflows/  # CI pipeline
├── CMakeLists.txt
├── Roadmap.md
├── State.md
├── Architecture.md
└── CHANGELOG.md
```

---

## License

GPL-3.0 (JUCE dependency)
