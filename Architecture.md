# UDS Architecture

> **Last Updated**: 2026-01-28 (Phase 5)

---

## System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    UDSAudioProcessor                        │
│  ┌─────────────┐   ┌─────────────┐   ┌─────────────────┐   │
│  │RoutingGraph │──▶│ DelayMatrix │──▶│ DelayBandNode×8 │   │
│  └─────────────┘   └─────────────┘   └─────────────────┘   │
│        ▲                                      │             │
│        │                                      ▼             │
│        │                              ┌───────────────┐     │
│        │                              │ SafetyLimiter │     │
│        │                              │  (8 stages)   │     │
│        │                              └───────────────┘     │
└────────┼────────────────────────────────────────────────────┘
         │
    UI Sync
         │
┌────────┴────────────────────────────────────────────────────┐
│                    UDSAudioProcessorEditor                  │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                   MainComponent                      │   │
│  │  ┌─────────────────┐  ┌─────────────────────────┐   │   │
│  │  │BandParameterPanel│  │   NodeEditorCanvas      │   │   │
│  │  │     ×8 (tabs)    │  │  ┌─────────────────┐    │   │   │
│  │  └─────────────────┘  │  │ BandNodeComponent×8│   │   │   │
│  │                        │  │ + Input/Output    │   │   │   │
│  │                        │  └─────────────────┘    │   │   │
│  │                        └─────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

---

## Core Components

### RoutingGraph
**Location**: `Source/Core/RoutingGraph.h`

Manages node connections and computes processing order.

| Method | Purpose |
|--------|---------|
| `connect(src, dst)` | Add connection |
| `disconnect(src, dst)` | Remove connection |
| `getProcessingOrder()` | Topological sort (Kahn's algorithm) |
| `getInputsFor(nodeId)` | Get source nodes |
| `wouldCreateCycle()` | Validation |

**Node IDs**: 0=Input, 1-8=Bands, 9=Output

---

### DelayMatrix
**Location**: `Source/Core/DelayMatrix.h`

Orchestrates 8 delay bands using routing graph.

| Method | Purpose |
|--------|---------|
| `prepare(sampleRate, blockSize)` | Allocate buffers |
| `processWithRouting(buffer, mix, graph)` | Process using external routing |
| `setBandParams(index, params)` | Update band parameters |

**Buffer Strategy**: Per-node buffers in `nodeBuffers_` map.

---

### DelayBandNode
**Location**: `Source/Core/DelayBandNode.h`

Single delay line with circular buffer.

| Parameter | Range | Default |
|-----------|-------|---------|
| delayTimeMs | 0-700 | 250 |
| feedback | 0-1 | 0.3 |
| level | 0-1 | 1.0 |
| pan | -1 to 1 | 0 |
| algorithm | Digital/Analog/Tape/LoFi | Digital |
| pingPong | true/false | false |

**Interpolation**: 4-point cubic Hermite for smooth modulated delays.

---

### DelayAlgorithm
**Location**: `Source/Core/DelayAlgorithm.h`

Polymorphic algorithm interface for per-band character:

| Algorithm | Character | Key DSP |
|-----------|-----------|--------|
| Digital | Clean, precise | Pass-through |
| Analog | Warm, saturated | tanh + LPF |
| Tape | Vintage, saturated | Jiles-Atherton hysteresis |
| Lo-Fi | Degraded | Bitcrush + sample rate reduction |

---

### SafetyLimiter
**Location**: `Source/Core/SafetyLimiter.h`

**8-Stage Audio Protection Chain** to protect equipment and hearing:

| Stage | Protection | Purpose |
|-------|-----------|---------|
| 0 | Extreme Peak Detection | Instant mute at +12dB |
| 1 | NaN/Inf Protection | Replace corrupt samples |
| 2 | DC Offset Blocking | 5Hz HPF prevents woofer damage |
| 3 | Safety Mute Check | Honor active mutes |
| 4 | Soft-Knee Limiting | Fast attack (0.1ms), slow release (50ms) |
| 5 | Sustained Loudness | Catch feedback runaway (500ms window) |
| 6 | Slew Rate Limiting | Prevent ultrasonic tweeter damage |
| 7 | Hard Clip | Final -1 to +1 bounds |

---

## UI Components

### NodeEditorCanvas
**Location**: `Source/UI/NodeEditorCanvas.h`

Visual cable patching interface.

| Feature | Implementation |
|---------|----------------|
| Cable drawing | Cubic bezier via `juce::Path` |
| Node dragging | Component mouse handlers |
| Zoom | `zoomFactor_` applied via `AffineTransform` |
| Pan | `panOffset_` via right-click drag |
| Undo/Redo | `RoutingUndoManager` with snapshot-based undo |

---

### BandParameterPanel
**Location**: `Source/UI/BandParameterPanel.h`

Per-band parameter controls.

| Feature | Implementation |
|---------|----------------|
| Rotary knobs | Time, Feedback, Level, Pan, Filter, LFO |
| Solo/Mute | Toggle buttons with exclusive solo |
| Signal LED | Activity indicator via `signalLevel_` |
| Double-click reset | All sliders reset to defaults |

---

## Data Flow

```
Audio Thread:
  Input Buffer
       │
       ▼
  RoutingGraph.getProcessingOrder()
       │
       ▼
  For each node in order:
    1. Sum inputs from source nodes
    2. Process through DelayBandNode (with algorithm)
    3. Apply FilterSection (hi/lo cut)
    4. Apply LFO modulation
    5. Apply ping-pong (if enabled)
    6. Store in nodeBuffers_[nodeId]
       │
       ▼
  SafetyLimiter (8-stage protection)
       │
       ▼
  Mix: dry + (wet × globalMix)
       │
       ▼
  Output Buffer
```

---

## Project Infrastructure

| File | Purpose |
|------|---------|
| CHANGELOG.md | Version history |
| .clang-format | Code style configuration |
| .github/workflows/ci.yml | Automated CI builds |
| Tests/DSPTests.cpp | Catch2 unit tests |
