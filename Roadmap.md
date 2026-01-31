# UDS Roadmap

> **Last Updated**: 2026-01-30  
> **Current Focus**: Phase 5 — Dynamic Band Count

*"The only delay where you can watch your sound travel"*

## Progress Overview

| Phase | Focus | Status |
|-------|-------|--------|
| 1 | Foundation | ✅ Complete |
| 2 | Visual Routing | ✅ Complete |
| 3 | Production Ready | ✅ Complete |
| 4 | Holdsworth Tribute | 🔶 3/4 Done |
| 5 | DSP Foundations | 🔶 In Progress |
| 6 | Algorithm Library | ⬜ Planned |
| 7 | Premium Features | ⬜ Planned |
| 8 | Commercial Release | ⬜ Planned |

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

| Preset Type | Details | Depends On | Status |
| ----------- | ------- | ---------- | ------ |
| Stereo Enhanced Lead (×9) | 10-40ms delays, hard-panned L/R, chorus mod | 3.2 LFO | ✅ |
| Single Source Point Stereo | Two delays, one phase-inverted | 3.2 Phase | ✅ |
| Vintage Echo (×3) | 200-400ms, filter darkening | 3.1 Analog, 3.2 Filters | ✅ |
| Volume Pedal Swell FX | Attack envelope, pad textures | 5.4 Attack Envelope | ⏳ |

### 4.2 Documentation

- [ ] Parameter comparison chart (UD Stomp → UDS)

---

## Phase 5: DSP Foundations

> *Architectural work that enables future features*

**Completed ✅**

- [x] Cubic Hermite interpolation (smooth modulated delays)
- [x] Signalsmith Stretch + chowdsp_wdf libraries
- [x] Jiles-Atherton hysteresis for Tape algorithm
- [x] Brownian Motion + Lorenz Attractor LFOs
- [x] Expression pedal mapping (MIDI CC learn, 77 parameters)

**Remaining**

- [ ] Attack envelope *(blocks: Phase 4 Volume Pedal Swell FX)*
- [ ] Algorithm plugin architecture *(blocks: Phase 6 new algorithms)*

### 5.5 Dynamic Band Count (IN PROGRESS)

> *Configurable 1-12 bands with right-click add/remove*

**Completed ✅**

- [x] MAX_BANDS=12 constant, 12 bands always allocated
- [x] 10-second max delay per band (up from 700ms)
- [x] Color arrays expanded to 12 bands

**In Progress**

- [ ] Active band tracking in RoutingGraph
- [ ] Right-click context menu: add/remove bands
- [ ] Band panel visibility (show/hide based on active count)
- [x] Display "8/12 Bands" count on main screen
- [ ] Preset serialization for active band list

---

## Phase 6: Algorithm Library

> ⚠️ **Blocked by**: Phase 5 Algorithm plugin architecture

### 6.1 Improve Existing Algorithms

| Algorithm | Current State | Improvements Needed |
|-----------|---------------|---------------------|
| Digital | Clean pass-through | Soft limiter, sub-octave |
| Analog | tanh + LPF | BBD companding, clock noise, filter drift |
| Tape | Jiles-Atherton ✅ | Wow/flutter, head response, tape speed |
| Lo-Fi | Bitcrush + SR | Vinyl crackle, radio filter, pitch warble |

### 6.2 New Algorithms (MVP)

| Algorithm | Description | Complexity |
|-----------|-------------|------------|
| **Reverse** | Reversed chunks with crossfade | Medium |
| **Ducking** | Ducks while playing, swells on silence | Easy |
| **Swell** | Auto-volume swells on repeats | Easy |
| **Freeze** | Capture buffer, loop infinitely | Medium |
| **Tape Stop** | Slowdown effect for transitions | Easy |

### 6.3 New Algorithms (Premium)

| Algorithm | Description | Complexity |
|-----------|-------------|------------|
| **Ice/Shimmer** | Pitch-shifted ethereal delays | Hard |
| **Multi-Head** | Space Echo simulation | Medium |
| **Sweep Echo** | Resonant filter sweep | Medium |
| **Pattern** | Rhythmic multi-tap patterns | Medium |
| **Granular** | Grain size, density, pitch | Hard |

---

## Phase 7: Premium Features

> *Features expected on Eventide/Strymon-tier delays*

### 7.1 Routing & Bypass

- [ ] Kill Dry mode (100% wet for parallel FX loops)
- [ ] Trails/Spillover (delays continue when bypassed)
- [ ] Analog dry path (zero-latency dry signal)

### 7.2 Modulation Enhancements

- [ ] Per-repeat filter sweep (resonant LPF/HPF/BPF)
- [ ] Per-repeat tremolo with LFO sync
- [ ] Grit/saturation control per algorithm
- [ ] Diffusion/Smear (allpass reverb textures)
- [ ] Tempo sync for generative LFOs

### 7.3 Global Features

- [x] Global tap tempo with subdivisions
- [x] Expression pedal mapping (MIDI CC learn, right-click assign)
- [ ] **Stereo width control** (missing from competitors)
- [ ] Preset morphing/crossfade

### 7.4 Workflow & UX

- [ ] **Copy/paste band settings** (right-click menu)
- [ ] Automation readout (show automated parameters)
- [ ] Oversampling toggle (CPU/quality tradeoff)

---

## Phase 8: Commercial Release

> *Required for $30+ product*

### 8.1 UI Polish

- [x] Preset browser with tag filtering
- [ ] Visual spectrum analyzer
- [ ] Modulation visualization
- [ ] Responsive layout
- [ ] Dark/light theme toggle

### 8.2 Product Completion

- [ ] AAX format (Pro Tools)
- [ ] CLAP format (future-proofing)
- [ ] PDF user manual
- [ ] 50+ factory presets
- [ ] Demo video / product page

### 8.3 Platform Notes

> ⚠️ AU format deferred - no Mac hardware for development  
> ⚠️ ARM/Apple Silicon deferred - no hardware  
> ⚠️ Linux VST3 low priority - small market, Wine wrappers common

### 8.4 Future Ideas (Post-Release)

- [ ] Visual signal flow (cables pulse with audio)
- [ ] Routing morphing (crossfade between configurations)
- [ ] Spectral splitting (route frequency bands to delays)
- [ ] Conway's Game of Life modulation

---

## Unique Selling Points

> *What makes UDS worth $30-50*

1. **12-Band Delay Matrix** - Configurable 1-12 independent delay lines with arbitrary routing
2. **Per-Band Algorithm Selection** - Mix Tape + Digital + Analog in one plugin
3. **Visual Routing Graph** - Modular-style patching with instant feedback
4. **Yamaha UD Stomp DNA** - Based on legendary hardware architecture
5. **77-Parameter Expression Control** - Full MIDI CC mapping with range training

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
