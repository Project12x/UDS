# Changelog

All notable changes to UDS - Universal Delay System will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **A/B Comparison** - Store/recall two preset states for instant comparison
- **Preset Tag Filtering** - Browse presets by category (Stereo Lead, Rhythmic, Vintage)
- **I/O Configuration selector** (Auto, Mono, Mono→Stereo, Stereo modes)
- **Master LFO "None" option** to disable master modulation
- Per-band ping-pong delay toggle (L/R alternation)
- Keyboard shortcuts: Ctrl+Z (Undo), Ctrl+Shift+Z (Redo) for routing
- Double-click to reset sliders to default values
- Slider tooltips on hover
- Enhanced audio safety protocols (DC blocking, silence detection)
- Unit test framework (Catch2)
- GitHub Actions CI workflow
- clang-format configuration

### Changed
- Parameter version bumped to 2 (invalidates old presets)
- Fixed deprecated Font constructor warnings (JUCE 8 FontOptions)

### Fixed
- Removed unused ledRadius variable
- Updated Roadmap checkboxes for completed features

---

## [0.1.0] - 2026-01-27

### Added
- 8 independent delay bands with arbitrary routing
- 4 delay algorithms: Digital, Analog, Tape, Lo-Fi
- Per-band filters (hi-cut/lo-cut)
- Per-band LFO modulation (Sine, Triangle, Saw, Square)
- Phase inversion per band
- Tempo sync with note divisions
- Solo/Mute per band
- Signal activity LED indicators
- Node-based visual routing editor with drag-and-drop cables
- Undo/Redo for routing changes
- Preset browser with save/load
- Standalone metronome for tempo sync testing
- Custom industrial UI with Space Grotesk typography
- Safety limiter with soft-knee compression

### Technical
- JUCE 8 framework
- C++17/20 standard
- CMake build system
- VST3 and Standalone builds
