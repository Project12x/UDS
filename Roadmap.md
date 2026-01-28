# UDS Roadmap

> **Last Updated**: 2026-01-27  
> **Status**: Phase 3 In Progress

*"The only delay where you can watch your sound travel"*

---

## Phase 1: Foundation ✅

| Sub | Focus | Status |
| --- | ----- | ------ |
| 1.1 | Circular buffer delay, 8-band parallel | ✅ |
| 1.2 | Basic parameter UI (2×4 grid) | ✅ |
| 1.3 | VST3/Standalone builds | ✅ |

---

## Phase 2: Visual Routing ✅

| Sub | Focus | Status |
| --- | ----- | ------ |
| 2.1 | Node editor canvas with bezier cables | ✅ |
| 2.2 | Draggable nodes, input/output ports | ✅ |
| 2.3 | RoutingGraph + topological sort | ✅ |
| 2.4 | Routing sync to audio engine | ✅ |
| 2.5 | Zoom/pan, per-band colors, cable deletion | ✅ |

---

## Phase 3: Production Ready

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
- [x] Factory preset curation (12 Holdsworth-inspired presets)
- [x] Preset browser with tags (category submenus)
- [x] A/B comparison (toggle button + menu)

### 3.4 UI Polish ✅

- [x] Per-band enable/bypass toggles
- [x] LookAndFeel hover effects
- [x] Algorithm selector dropdown
- [x] Solo/mute buttons per band

### 3.5 Engineering Best Practices ✅

- [x] CHANGELOG.md
- [x] .clang-format configuration
- [x] GitHub Actions CI workflow
- [x] Catch2 unit test scaffold
- [x] 8-stage SafetyLimiter (equipment/hearing protection)
- [x] Zero-warning builds

### 3.6 I/O & UX

- [x] Mono In / Mono Out
- [x] Mono In / Stereo Out (Stereo Expander)
- [x] Stereo In / Stereo Out
- [x] Signal activity indicator
- [x] Tempo sync modes
- [x] Undo/redo for routing changes
- [x] Keyboard shortcuts (Ctrl+Z, Ctrl+Shift+Z)
- [x] Double-click to reset sliders
- [x] Slider tooltips

---

## Phase 4: Holdsworth Tribute

> *Allan Holdsworth programmed the first 27 UD Stomp factory presets*

### 4.1 Preset Recreation

| Preset Type | Details | Depends On |
| ----------- | ------- | ---------- |
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

### 5.5 Generative Modulation (The "Living" System)

#### Implemented ✅

- [x] Brownian Motion LFO (Random Walk with smooth interpolation)
- [x] Lorenz Attractor (Chaotic modulation with slew limiting)
- [x] Rate control for generative evolution speed

#### Planned: Musical Quantization

- [ ] Tempo Sync Mode - Lock generative updates to host BPM
- [ ] Note Division Selector - Quantize to 1/4, 1/8, 1/16, 1/32 notes
- [ ] Freeform vs Quantized Toggle - Per-LFO choice
- [ ] Swing/Shuffle - Humanize quantized timing
- [ ] Conway's Game of Life (Grid-based modulation source)

### 5.6 Advanced Algorithms (New Algorithm Types)

- [ ] Granular Delay (Grain size, density, pitch)
- [ ] Diffusion/Smear (Allpass filters for reverb textures)
- [ ] Pitch Shifting (Crystals/Shimmer effect)

### 5.7 Algorithm Refinement & DL4 Expansion

> *Improve existing algorithms toward professional hardware quality and expand DL4-style algorithm library*

#### Phase A: DSP Quality Integration ✅

> Added **Signalsmith Stretch** (MIT) and **chowdsp_wdf** (BSD-3) libraries. Implemented **cubic Hermite interpolation** and **Jiles-Atherton hysteresis** for tape saturation.

- [x] Add `signalsmith-audio/stretch` via CMake FetchContent
- [x] Add `chowdsp_wdf` via CMake FetchContent
- [x] Replace linear interpolation with cubic Hermite in `DelayBandNode`
- [x] Implement Jiles-Atherton hysteresis for Tape algorithm

#### Phase B: Improve Existing Delay Types

> Each algorithm should have distinct, musical character

**Digital** (currently clean pass-through)
- [ ] Add optional soft-knee limiter for clean feedback control
- [ ] Optional sub-octave generation for bass enhancement

**Analog** (currently tanh saturation + LPF)
- [ ] Add bucket-brigade companding artifacts
- [ ] Clock noise simulation at high feedback
- [ ] Variable filter drift/instability

**Tape** (now has Jiles-Atherton hysteresis)
- [ ] Add subtle wow/flutter via internal LFO
- [ ] Head gap frequency response modeling
- [ ] Tape speed variations (15ips vs 7.5ips character)

**Lo-Fi** (currently bitcrush + sample rate reduction)
- [ ] Add vinyl-style crackle/noise
- [ ] Telephone/radio bandpass filter option
- [ ] Pitch instability/warble

#### Phase C: DL4 Algorithm Expansion

> New algorithms inspired by Line 6 DL4 MKII

- [ ] **Reverse** - Reversed audio chunks with crossfade
- [ ] **Sweep Echo** - Delay with resonant filter sweep
- [ ] **Tube Echo** - Emulate tube-based Echoplex character
- [ ] **Multi-Head** - Simulated multi-head tape like Roland Space Echo
- [ ] **Pattern** - Rhythmic multi-tap patterns
- [ ] **Swell** - Auto-volume swells on repeats
- [ ] **Ducking** - Delay ducks while playing, swells on silence
- [ ] **Ice/Shimmer** - Pitch-shifted ethereal delays (Crystals-style)
- [ ] **Trem** - Synchronized tremolo on repeats
- [ ] **Filter** - Resonant filter sweep on repeats

#### Phase D: Premium Feature Parity

> Features expected on Eventide/Strymon-tier delays

**Routing & Bypass**
- [ ] Kill Dry mode (100% wet for parallel FX loops)
- [ ] Trails/Spillover (delays continue when bypassed)
- [ ] Analog dry path option (zero-latency dry signal)

**Modulation Enhancements**
- [ ] Per-repeat filter sweep (resonant LPF/HPF/BPF)
- [ ] Per-repeat tremolo with LFO sync
- [ ] Grit/saturation control per algorithm

**Global Features**
- [x] Global tap tempo with subdivisions
- [ ] Expression pedal mapping (MIDI CC learn)
- [ ] Preset morphing/crossfade

#### Quality Benchmarks

- [ ] Reference A/B Testing - Compare against Eventide TimeFactor, Strymon Timeline
- [ ] Modulation Response - Confirm modulated delays smooth with cubic interpolation

#### Phase E: Commercial Polish ($30+ Value)

> Features expected at premium price point

**DSP Features**
- [ ] **Freeze** - Capture delay buffer, loop infinitely with decay control
- [ ] **Rhythm patterns** - Programmable multi-tap patterns like EchoBoy
- [ ] **Diffusion** - Smear repeats into reverb-like textures

**UI/UX**
- [ ] Preset browser with tag filtering and search (✅ tags done)
- [ ] Visual spectrum analyzer on output
- [ ] Modulation visualization (show LFO affecting parameters)
- [ ] Responsive layout for different window sizes
- [ ] Dark/light theme toggle

**Product Completion**
- [ ] AAX format for Pro Tools compatibility
- [ ] PDF user manual with algorithm descriptions
- [ ] Preset library (50+ factory presets covering all algorithms)
- [ ] Demo video / product page content

---

### Unique Selling Points

> *What makes UDS worth $30-50*

1. **8-Band Delay Matrix** - No other plugin offers 8 independent delay lines with arbitrary routing
2. **Per-Band Algorithm Selection** - Mix Tape + Digital + Analog in one plugin
3. **Visual Routing Graph** - Modular-style patching with instant feedback
4. **Yamaha UD Stomp DNA** - Based on legendary hardware architecture

---

## Maintenance Protocol

### Commit Triggers (Do ALL when ANY trigger fires)

**Trigger 1: Feature Complete** - A roadmap checkbox can be marked `[x]`  
**Trigger 2: 5+ Files Changed** - Accumulated edits across multiple files  
**Trigger 3: User Requests** - User says "commit", "save", "push", etc.  
**Trigger 4: Before Risky Change** - About to refactor or delete significant code  
**Trigger 5: End of Conversation** - User says goodbye/thanks/done

**Commit Actions:**
- [ ] `git add -A && git commit -m "descriptive message"`
- [ ] `git push origin main`
- [ ] Update **State.md** with current progress

### Documentation Triggers

**After completing a roadmap phase (A, B, C, D, E):**
- [ ] Update **CHANGELOG.md** with version bump
- [ ] Update **README.md** if user-facing features changed  
- [ ] Update **Architecture.md** if new classes/modules added
- [ ] Mark roadmap items `[x]` complete
- [ ] Create git tag: `git tag -a v0.X.0 -m "Phase X complete"`

### Code Hygiene Triggers

**After 3 features OR 500+ lines changed OR before major refactor:**
- [ ] Comment pass - Add/update documentation comments
- [ ] Dead code removal - Delete unused functions/variables
- [ ] Warning cleanup - Address compiler warnings  
- [ ] Test coverage - Add tests for new code paths

### Quarterly Review (Every 3 months)

- [ ] Dependency updates (JUCE, libraries)
- [ ] Performance profiling
- [ ] Memory leak check

---

## Quick Links

| Doc | Purpose | Update Frequency |
| --- | ------- | ---------------- |
| [State.md](State.md) | Current snapshot | Per-session |
| [Architecture.md](Architecture.md) | System design | Per-phase |
| [README.md](README.md) | Build instructions | When features change |
| [CHANGELOG.md](CHANGELOG.md) | Version history | Per-phase |
| [Roadmap.md](Roadmap.md) | Development plan | Per-phase |
