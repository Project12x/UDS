#pragma once

#include <cmath>
#include <vector>

namespace uds {

/**
 * @brief Algorithm types for delay character
 */
enum class DelayAlgorithmType {
  Digital, // Clean, precise, transparent
  Analog,  // Subtle saturation, filter drift
  Tape,    // Wow/flutter, head saturation
  LoFi     // Bitcrushing, noise
};

/**
 * @brief Base interface for delay algorithms
 *
 * Each algorithm processes the feedback path differently to create
 * unique delay character. The core delay line is handled by DelayBandNode;
 * algorithms only process the signal within the feedback loop.
 */
class DelayAlgorithm {
public:
  virtual ~DelayAlgorithm() = default;

  /**
   * @brief Prepare the algorithm for processing
   * @param sampleRate Current sample rate
   */
  virtual void prepare(double sampleRate) = 0;

  /**
   * @brief Reset internal state
   */
  virtual void reset() = 0;

  /**
   * @brief Process a single sample through the algorithm
   *
   * This is called on samples in the feedback path to add character.
   *
   * @param sample Input sample
   * @return Processed sample with algorithm character applied
   */
  virtual float processSample(float sample) = 0;

  /**
   * @brief Get the algorithm type
   */
  virtual DelayAlgorithmType getType() const = 0;

  /**
   * @brief Get display name for UI
   */
  virtual const char* getName() const = 0;
};

/**
 * @brief Digital delay - clean, transparent, no coloration
 *
 * This is the "purist" delay with no processing in the feedback path.
 * Maximum clarity and precision.
 */
class DigitalDelay : public DelayAlgorithm {
public:
  void prepare(double /*sampleRate*/) override {
    // No state needed for digital
  }

  void reset() override {
    // Nothing to reset
  }

  float processSample(float sample) override {
    // Pass through unchanged - digital is clean
    return sample;
  }

  DelayAlgorithmType getType() const override {
    return DelayAlgorithmType::Digital;
  }

  const char* getName() const override { return "Digital"; }
};

/**
 * @brief Analog delay - subtle saturation and filter drift
 *
 * Emulates BBD (bucket brigade) style delays with:
 * - Soft saturation on feedback
 * - Subtle high-frequency rolloff
 * - Slight noise floor
 */
class AnalogDelay : public DelayAlgorithm {
public:
  void prepare(double sampleRate) override {
    sampleRate_ = sampleRate;

    // Simple one-pole lowpass for HF rolloff
    // fc ~= 8kHz, gives warm analog character
    float fc = 8000.0f;
    float wc = 2.0f * 3.14159f * fc / static_cast<float>(sampleRate);
    lpfCoeff_ = wc / (1.0f + wc);

    reset();
  }

  void reset() override { lpfState_ = 0.0f; }

  float processSample(float sample) override {
    // Soft saturation (tanh-style)
    float saturated = std::tanh(sample * 1.2f) * 0.9f;

    // One-pole lowpass filter (HF rolloff)
    lpfState_ += lpfCoeff_ * (saturated - lpfState_);

    return lpfState_;
  }

  DelayAlgorithmType getType() const override {
    return DelayAlgorithmType::Analog;
  }

  const char* getName() const override { return "Analog"; }

private:
  double sampleRate_ = 44100.0;
  float lpfCoeff_ = 0.5f;
  float lpfState_ = 0.0f;
};

/**
 * @brief Tape delay - Jiles-Atherton hysteresis and head saturation
 *
 * Emulates tape echo machines with:
 * - Jiles-Atherton magnetic hysteresis (history-dependent saturation)
 * - Tape head high-frequency loss
 * - Authentic "warmth" from asymmetric saturation
 *
 * Algorithm based on Jatin Chowdhury's published research (public domain math)
 */
class TapeDelay : public DelayAlgorithm {
public:
  void prepare(double sampleRate) override {
    sampleRate_ = sampleRate;
    T_ = 1.0f / static_cast<float>(sampleRate);

    // Jiles-Atherton parameters (tuned for tape character)
    Ms_ = 0.5f;     // Saturation magnetization
    a_ = 350.0f;    // Shape parameter (affects sharpness)
    c_ = 1.7f;      // Domain wall coupling
    k_ = 40.0f;     // Coercivity (affects hysteresis loop width)
    alpha_ = 0.01f; // Inter-domain coupling

    // Lowpass for tape head HF loss (6kHz cutoff)
    float fc = 6000.0f;
    float wc = 2.0f * 3.14159f * fc / static_cast<float>(sampleRate);
    lpfCoeff_ = wc / (1.0f + wc);

    reset();
  }

  void reset() override {
    M_prev_ = 0.0f;
    H_prev_ = 0.0f;
    lpfState_ = 0.0f;
  }

  float processSample(float sample) override {
    // Scale input to magnetic field strength H
    float H = sample * 1000.0f; // Input gain for hysteresis

    // Langevin function: L(x) = coth(x) - 1/x
    float Q = (H + alpha_ * M_prev_) / a_;
    float L;
    if (std::abs(Q) < 0.001f) {
      L = Q / 3.0f; // Taylor expansion for small x
    } else {
      L = 1.0f / std::tanh(Q) - 1.0f / Q;
    }

    // Anhysteretic magnetization
    float M_an = Ms_ * L;

    // Calculate delta M (simplified real-time solver)
    float dH = H - H_prev_;
    float delta = (dH > 0.0f) ? 1.0f : -1.0f;

    // Irreversible magnetization component
    float dM_irr = (M_an - M_prev_) / (k_ * delta * (1.0f - c_) +
                                       c_ * (M_an - M_prev_) / a_ + 1e-6f);

    // Update magnetization with bounded rate
    float M = M_prev_ + dM_irr * std::abs(dH) * T_ * 1000.0f;
    M = std::clamp(M, -Ms_, Ms_);

    // Store states for next sample
    M_prev_ = M;
    H_prev_ = H;

    // Normalize output and apply lowpass (tape head HF loss)
    float output = M / Ms_ * 0.85f;
    lpfState_ += lpfCoeff_ * (output - lpfState_);

    return lpfState_;
  }

  DelayAlgorithmType getType() const override {
    return DelayAlgorithmType::Tape;
  }

  const char* getName() const override { return "Tape"; }

private:
  double sampleRate_ = 44100.0;
  float T_ = 1.0f / 44100.0f; // Sample period

  // Jiles-Atherton parameters
  float Ms_ = 0.5f;     // Saturation magnetization
  float a_ = 350.0f;    // Shape parameter
  float c_ = 1.7f;      // Domain wall coupling
  float k_ = 40.0f;     // Coercivity
  float alpha_ = 0.01f; // Inter-domain coupling

  // State
  float M_prev_ = 0.0f; // Previous magnetization
  float H_prev_ = 0.0f; // Previous field
  float lpfState_ = 0.0f;
  float lpfCoeff_ = 0.5f;
};

/**
 * @brief Lo-Fi delay - bitcrushing and noise
 *
 * Creates degraded, vintage digital character with:
 * - Bit depth reduction
 * - Sample rate reduction
 * - Added noise floor
 */
class LoFiDelay : public DelayAlgorithm {
public:
  void prepare(double sampleRate) override {
    sampleRate_ = sampleRate;
    reset();
  }

  void reset() override {
    holdSample_ = 0.0f;
    holdCounter_ = 0;
  }

  float processSample(float sample) override {
    // Sample rate reduction (hold every N samples)
    const int decimation = 4; // Effective ~11kHz at 44.1kHz
    if (++holdCounter_ >= decimation) {
      holdCounter_ = 0;

      // Bit depth reduction (simulate 12-bit)
      const float levels = 4096.0f;
      holdSample_ = std::round(sample * levels) / levels;
    }

    // Add subtle noise floor
    float noise = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.002f;

    return holdSample_ + noise;
  }

  DelayAlgorithmType getType() const override {
    return DelayAlgorithmType::LoFi;
  }

  const char* getName() const override { return "Lo-Fi"; }

private:
  double sampleRate_ = 44100.0;
  float holdSample_ = 0.0f;
  int holdCounter_ = 0;
};

/**
 * @brief Factory for creating delay algorithms
 */
inline std::unique_ptr<DelayAlgorithm>
createDelayAlgorithm(DelayAlgorithmType type) {
  switch (type) {
  case DelayAlgorithmType::Digital:
    return std::make_unique<DigitalDelay>();
  case DelayAlgorithmType::Analog:
    return std::make_unique<AnalogDelay>();
  case DelayAlgorithmType::Tape:
    return std::make_unique<TapeDelay>();
  case DelayAlgorithmType::LoFi:
    return std::make_unique<LoFiDelay>();
  default:
    return std::make_unique<DigitalDelay>();
  }
}

} // namespace uds
