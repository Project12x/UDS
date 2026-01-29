# UDS Manual Testing Checklist

> **Status**: 🟡 Supplement to Automated Tests  
> **Last Updated**: 2026-01-28  
> **Note**: Most tests are now automated via Catch2. This checklist is for **subjective audio quality** and **GUI behavior** that cannot be automated.

---

## Automated Tests ✅

Run with:
```powershell
cmake -B build-test -S . -DUDS_BUILD_TESTS=ON
cmake --build build-test --config Release --target UDS_Tests
.\build-test\Tests\Release\UDS_Tests.exe
```

**Last Run:** 312,341 assertions passed across 9 test cases

---

## 1. Subjective Audio Quality (Manual)

### Digital Delay
- [ ] Clean, uncolored repeats
- [ ] Full bandwidth (no filtering artifacts)
- [ ] Stable at high feedback (>90%)

### Analog Delay
- [ ] High-frequency rolloff per repeat
- [ ] Subtle saturation/warmth (tanh)
- [ ] Feedback decay sounds natural

### Tape Delay (Jiles-Atherton) ✅
- [ ] **Hysteresis saturation audible** (new)
- [ ] Progressive HF loss per repeat
- [ ] "Aged" character at high feedback
- [ ] Wow/flutter modulation (TODO: not yet implemented)

### Lo-Fi Delay
- [ ] Bitcrushing/sample rate reduction
- [ ] Noticeable aliasing artifacts
- [ ] Gritty/degraded texture

---

## 2. Routing Graph (Covered by Automated Tests)

> ✅ See `DSPTests.cpp` - routing validation is automated

---

## 3. Per-Band Parameters (Covered by Automated Tests)

> ✅ See `DSPTests.cpp` - parameter ranges are automated

---

## 4. LFO Modulation (Manual Verification Needed)

### Smooth Modulation
- [ ] Sine - smooth pitch wobble
- [ ] Triangle - linear ramp
- [ ] Sawtooth - one-direction sweep
- [ ] Square - stepped modulation

### Generative LFOs ✅
- [ ] Brownian Motion - random walk character
- [ ] Lorenz Attractor - chaotic but musical

### Interpolation Quality ✅
- [ ] **Cubic Hermite** prevents zipper noise (new)
- [ ] Smooth delay time changes with active LFO

---

## 5. GUI Testing (Manual)

### Controls
- [ ] All knobs respond to mouse
- [ ] Double-click resets to default
- [ ] Tooltips show on hover
- [ ] Value displays update correctly

### Node Editor
- [ ] Cables connect/disconnect
- [ ] Bezier curves render correctly
- [ ] Zoom (mouse wheel) works
- [ ] Pan (right-click drag) works
- [ ] Undo/Redo (Ctrl+Z / Ctrl+Shift+Z)

### Preset Browser
- [ ] All factory presets load
- [ ] Tag filtering works
- [ ] A/B comparison switches instantly
- [ ] Save/Load user presets

---

## 6. Holdsworth Preset Accuracy

> Compare to original MagicStomp parameters

- [ ] "Stereo U D Lead" - matches original timing
- [ ] "Mono Pong Lead" - ping-pong behavior correct
- [ ] "Vintage Plate" - reverb-like character
- [ ] All 12 presets load without errors

---

## 7. Stability (Manual Stress Test)

- [ ] Rapid preset switching (no crashes)
- [ ] Sample rate changes (44.1k → 48k → 96k)
- [ ] Buffer size changes during playback
- [ ] Extended use (30+ minutes, no memory leaks)
- [ ] High CPU scenario (all 8 bands, max feedback)

---

## Tests Superseded by Automation

The following were originally manual tests but are now covered by `DSPTests.cpp`:

- ~~Routing graph validation~~ → Automated
- ~~Parameter ranges~~ → Automated  
- ~~Tempo sync calculations~~ → Automated
- ~~Safety limiter behavior~~ → Automated
