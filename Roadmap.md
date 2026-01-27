# UDS Roadmap

> **Last Updated**: 2026-01-27  
> **Status**: Phase 3.4 Complete

*"The only delay where you can watch your sound travel"*

---

## Phase 1: Foundation ✅

| Sub | Focus | Status |
|-----|-------|--------|
| 1.1 | Circular buffer delay, 8-band parallel | ✅ |
| 1.2 | Basic parameter UI (2×4 grid) | ✅ |
| 1.3 | VST3/Standalone builds | ✅ |

---

## Phase 2: Visual Routing ✅

| Sub | Focus | Status |
|-----|-------|--------|
| 2.1 | Node editor canvas with bezier cables | ✅ |
| 2.2 | Draggable nodes, input/output ports | ✅ |
| 2.3 | RoutingGraph + topological sort | ✅ |
| 2.4 | Routing sync to audio engine | ✅ |
| 2.5 | Zoom/pan, per-band colors, cable deletion | ✅ |

---

## Phase 3: Production Ready 🔜

### 3.1 Delay Algorithms ✅

- [x] Digital (clean, precise)
- [x] Analog (subtle saturation, filter drift)
- [x] Tape (wow/flutter, head saturation)
- [x] Lo-Fi (bitcrushing, noise)

### 3.2 Filters & Modulation ✅

- [x] Hi-cut / lo-cut filters in DelayBandNode
- [x] Per-band filter controls in UI
- [x] LFO modulation on delay time
- [x] Individual LFO per band (Sine, Triangle, Saw, Square)
- [x] Per-band phase inversion toggle
- [x] Per-band ping-pong delay toggle

### 3.3 Preset System ✅

- [x] State serialization for routing
- [x] XML preset format
- [x] Basic preset browser
- [ ] Factory preset curation (Verify routing & MagicStomp accuracy)
- [ ] Preset browser with tags
- [ ] A/B comparison

### 3.4 UI Polish ✅

- [x] Per-band enable/bypass toggles
- [x] LookAndFeel hover effects
- [x] Algorithm selector dropdown
- [x] Solo/mute buttons per band

### 3.6 I/O Configurations

- [ ] Mono In / Mono Out
- [ ] Mono In / Stereo Out (Stereo Expander)
- [ ] Stereo In / Stereo Out
- [x] Signal activity indicator
- [x] Tempo sync modes
- [x] Undo/redo for routing changes
- [x] Per-band ping-pong delay toggle
- [x] Keyboard shortcuts (Ctrl+Z, Ctrl+Shift+Z)
- [x] Double-click to reset sliders
- [x] Slider tooltips
- [ ] Randomize routing button

### 3.5 Engineering Best Practices ✅

- [x] CHANGELOG.md
- [x] .clang-format configuration
- [x] GitHub Actions CI workflow
- [x] Catch2 unit test scaffold
- [x] 8-stage SafetyLimiter (equipment/hearing protection)
- [x] Zero-warning builds

---

## Phase 4: Holdsworth Tribute

> *Allan Holdsworth programmed the first 27 UD Stomp factory presets*

### 4.1 Preset Recreation

| Preset Type | Details | Depends On |
|-------------|---------|------------|
| Stereo Enhanced Lead (×9) | 10-40ms delays, hard-panned L/R, chorus mod | 3.2 LFO |
| Single Source Point Stereo | Two delays, one phase-inverted | 3.2 Phase |
| Vintage Echo (×3) | 200-400ms, filter darkening | 3.1 Analog, 3.2 Filters |
| Volume Pedal Swell FX | Attack envelope, pad textures | New: attack envelope |

### 4.2 Documentation

- [ ] Parameter comparison chart (UD Stomp → UDS)

---

## Phase 5: Signature Features

### 5.1 Visual Signal Flow ⭐

- [ ] Cables glow/pulse with audio level
- [ ] Real-time visualization of signal path

### 5.2 Routing Morphing

- [ ] Crossfade between routing configurations
- [ ] Tempo-synced routing changes
- [ ] Morph presets (A→B interpolation)

### 5.3 Spectral Splitting

- [ ] Route frequency bands to different delays
- [ ] Crossover frequency controls

### 5.4 Extended DSP

- [ ] Per-band input gain staging (pre-delay saturation)
- [ ] Sidechain ducking
- [ ] MIDI CC mapping
- [ ] Reverse delays
- [ ] Freeze/hold mode

---

## Future Development

*No committed timeline*

- MIDI note-to-delay mapping
- Granular delay mode
- Convolution room simulation
- MPE support

---

## Maintenance Protocol

**Each sub-phase**: Update State.md, comment pass, verify build  
**Each major phase**: Update Architecture.md, walkthrough.md, Roadmap.md

---

## Quick Links

| Doc | Purpose |
|-----|---------|
| [State.md](State.md) | Current snapshot |
| [Architecture.md](Architecture.md) | System design |
| [README.md](README.md) | Build instructions |
| [CHANGELOG.md](CHANGELOG.md) | Version history |
