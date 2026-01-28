# UDS Handoff Document

> **Created**: 2026-01-28  
> **Purpose**: Quick context for new AI sessions or collaborators

---

## One-Sentence Summary

UDS is an 8-band visual delay matrix VST3 plugin inspired by the Yamaha UD Stomp, featuring modular-style cable patching and Allan Holdsworth tribute presets.

---

## What This Project Is

- **Audio plugin** (VST3/Standalone) written in C++20 with JUCE 8
- **8 independent delay lines** with arbitrary routing between them
- **Visual node-based routing** like a modular synth
- **Commercial product** targeting $30-59 price point
- **Closed-source** - using only MIT/BSD-3/public domain libraries

---

## Key Files to Read First

| Priority | File | What You'll Learn |
|----------|------|-------------------|
| 1 | `State.md` | Current status, what works, what doesn't |
| 2 | `Roadmap.md` | Development phases and priorities |
| 3 | `Architecture.md` | Code structure and data flow |
| 4 | `Source/Core/DelayBandNode.h` | Core DSP logic |
| 5 | `Source/Core/RoutingGraph.h` | How routing works |

---

## Important Design Decisions

### Already Made (Don't Revisit)

1. **8-band matrix is core USP** - Don't reduce to fewer bands
2. **No looper integration** - User has separate looper project
3. **Closed-source commercial** - No GPL libraries except JUCE
4. **No AU format** - User has no Mac hardware
5. **Visual routing is essential** - Not just a technical feature, it's the selling point
6. **Holdsworth tribute presets** - Load from MagicStomp parser

### Architectural Choices

1. **Cubic Hermite interpolation** for modulated delays (already implemented)
2. **Jiles-Atherton hysteresis** for tape saturation (already implemented)
3. **8-stage SafetyLimiter** protects equipment (already implemented)
4. **Per-band algorithm selection** - Each of 8 bands can be Digital/Analog/Tape/Lo-Fi

---

## What's In Progress

| Item | Status | Notes |
|------|--------|-------|
| Roadmap refactor | Done, uncommitted | Split Phase 5 into 5-8 |
| DSP quality libraries | ✅ Integrated | Signalsmith, chowdsp_wdf |
| Attack envelope | Not started | Blocks Volume Swell presets |
| Parameter chart | Not started | UD Stomp → UDS mapping |

---

## Common Tasks

### Build the Project

```powershell
cd "c:\Users\estee\Desktop\My Stuff\Code\Antigravity\UDS"
cmake -B build -S .
cmake --build build --config Release
```

### Run Tests

```powershell
cmake --build build --config Release --target UDS_Tests
.\build\Tests\Release\UDS_Tests.exe
```

### Check Git Status

```powershell
git status
git log -5 --oneline
```

---

## Key Classes

| Class | File | Responsibility |
|-------|------|----------------|
| `DelayMatrix` | `Core/DelayMatrix.h` | Orchestrates all 8 bands |
| `DelayBandNode` | `Core/DelayBandNode.h` | Single delay line with algorithm |
| `RoutingGraph` | `Core/RoutingGraph.h` | Node connections, topological sort |
| `SafetyLimiter` | `Core/SafetyLimiter.h` | 8-stage audio protection |
| `PresetManager` | `Core/PresetManager.h` | XML preset serialization |
| `NodeEditorCanvas` | `UI/NodeEditorCanvas.h` | Visual cable patching |
| `BandParameterPanel` | `UI/BandParameterPanel.h` | Per-band knob controls |

---

## Maintenance Protocol

Per the roadmap, commit when ANY of these triggers fire:

1. **Feature complete** - A roadmap checkbox can be marked `[x]`
2. **5+ files changed** - Accumulated edits
3. **User requests** - "commit", "save", "push"
4. **Before risky change** - Major refactor or deletion
5. **End of conversation** - User says goodbye/thanks/done

---

## Platform Limitations

| Platform | Status | Reason |
|----------|--------|--------|
| Windows VST3 | ✅ Primary | User's dev machine |
| Windows Standalone | ✅ Works | Built automatically |
| AAX | Planned | Phase 8 |
| AU (Mac) | Deferred | No Mac hardware |
| ARM/Apple Silicon | Deferred | No hardware |
| Linux | Low priority | Small market, Wine wrappers common |
| CLAP | Planned | Phase 8, future-proofing |

---

## Knowledge Base

Check the Antigravity knowledge base for:
- `uds_delay_emulation_project` - Comprehensive technical docs
- `modern_juce_audio_development` - JUCE patterns and practices
- `software_portfolio_strategy` - Commercial context

---

## Contact Context

The human developer ("estee") is:
- Working on multiple audio projects (Pedalboard3, LoopDeLoop)
- Focused on commercial viability ($30-50 plugins)
- Prefers direct answers over verbose explanations
- Windows-only development environment
- Guitar-focused (Holdsworth influence)
