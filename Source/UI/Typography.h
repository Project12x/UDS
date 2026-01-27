#pragma once

#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace uds {

/**
 * @brief Typography system for UDS
 *
 * Font hierarchy:
 * - Space Grotesk Bold: Titles, headers
 * - Space Grotesk Medium: Button labels, emphasis
 * - Space Grotesk Regular: Labels, body text
 * - JetBrains Mono: Numerical values, parameter readouts
 */
class Typography {
public:
  static Typography &getInstance() {
    static Typography instance;
    return instance;
  }

  // Primary typefaces
  juce::Typeface::Ptr getSpaceGroteskRegular() const {
    return spaceGroteskRegular_;
  }
  juce::Typeface::Ptr getSpaceGroteskMedium() const {
    return spaceGroteskMedium_;
  }
  juce::Typeface::Ptr getSpaceGroteskBold() const { return spaceGroteskBold_; }
  juce::Typeface::Ptr getJetBrainsMono() const { return jetBrainsMono_; }

  // Legacy Inter typefaces (for backward compatibility if needed)
  juce::Typeface::Ptr getInterRegular() const { return interRegular_; }
  juce::Typeface::Ptr getInterMedium() const { return interMedium_; }
  juce::Typeface::Ptr getInterBold() const { return interBold_; }

  // Pre-configured fonts for common use cases

  // Titles - Bold, large
  juce::Font getTitleFont(float height = 18.0f) const {
    if (spaceGroteskBold_)
      return juce::Font(
          juce::FontOptions(spaceGroteskBold_).withHeight(height));
    return juce::Font(juce::FontOptions().withHeight(height));
  }

  // Headers - Medium weight
  juce::Font getHeaderFont(float height = 14.0f) const {
    if (spaceGroteskMedium_)
      return juce::Font(
          juce::FontOptions(spaceGroteskMedium_).withHeight(height));
    return juce::Font(juce::FontOptions().withHeight(height));
  }

  // Labels - Regular weight
  juce::Font getLabelFont(float height = 12.0f) const {
    if (spaceGroteskRegular_)
      return juce::Font(
          juce::FontOptions(spaceGroteskRegular_).withHeight(height));
    return juce::Font(juce::FontOptions().withHeight(height));
  }

  // Button text - Medium, slightly larger
  juce::Font getButtonFont(float height = 11.0f) const {
    if (spaceGroteskMedium_)
      return juce::Font(
          juce::FontOptions(spaceGroteskMedium_).withHeight(height));
    return juce::Font(juce::FontOptions().withHeight(height));
  }

  // Parameter values - Mono, for numbers
  juce::Font getValueFont(float height = 11.0f) const {
    if (jetBrainsMono_)
      return juce::Font(juce::FontOptions(jetBrainsMono_).withHeight(height));
    return juce::Font(juce::FontOptions().withHeight(height));
  }

  // Small captions
  juce::Font getCaptionFont(float height = 10.0f) const {
    if (spaceGroteskRegular_)
      return juce::Font(
          juce::FontOptions(spaceGroteskRegular_).withHeight(height));
    return juce::Font(juce::FontOptions().withHeight(height));
  }

private:
  Typography() {
    // Load Space Grotesk fonts with null checks
    if (BinaryData::SpaceGroteskRegular_ttf != nullptr &&
        BinaryData::SpaceGroteskRegular_ttfSize > 0) {
      spaceGroteskRegular_ = juce::Typeface::createSystemTypefaceFor(
          BinaryData::SpaceGroteskRegular_ttf,
          BinaryData::SpaceGroteskRegular_ttfSize);
    }

    if (BinaryData::SpaceGroteskMedium_ttf != nullptr &&
        BinaryData::SpaceGroteskMedium_ttfSize > 0) {
      spaceGroteskMedium_ = juce::Typeface::createSystemTypefaceFor(
          BinaryData::SpaceGroteskMedium_ttf,
          BinaryData::SpaceGroteskMedium_ttfSize);
    }

    if (BinaryData::SpaceGroteskBold_ttf != nullptr &&
        BinaryData::SpaceGroteskBold_ttfSize > 0) {
      spaceGroteskBold_ = juce::Typeface::createSystemTypefaceFor(
          BinaryData::SpaceGroteskBold_ttf,
          BinaryData::SpaceGroteskBold_ttfSize);
    }

    // Load JetBrains Mono for values/numbers
    if (BinaryData::JetBrainsMonoRegular_ttf != nullptr &&
        BinaryData::JetBrainsMonoRegular_ttfSize > 0) {
      jetBrainsMono_ = juce::Typeface::createSystemTypefaceFor(
          BinaryData::JetBrainsMonoRegular_ttf,
          BinaryData::JetBrainsMonoRegular_ttfSize);
    }

    // Load Inter fonts (kept for backward compatibility)
    if (BinaryData::InterRegular_ttf != nullptr &&
        BinaryData::InterRegular_ttfSize > 0) {
      interRegular_ = juce::Typeface::createSystemTypefaceFor(
          BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize);
    }

    if (BinaryData::InterMedium_ttf != nullptr &&
        BinaryData::InterMedium_ttfSize > 0) {
      interMedium_ = juce::Typeface::createSystemTypefaceFor(
          BinaryData::InterMedium_ttf, BinaryData::InterMedium_ttfSize);
    }

    if (BinaryData::InterBold_ttf != nullptr &&
        BinaryData::InterBold_ttfSize > 0) {
      interBold_ = juce::Typeface::createSystemTypefaceFor(
          BinaryData::InterBold_ttf, BinaryData::InterBold_ttfSize);
    }
  }

  // Space Grotesk primary typefaces
  juce::Typeface::Ptr spaceGroteskRegular_;
  juce::Typeface::Ptr spaceGroteskMedium_;
  juce::Typeface::Ptr spaceGroteskBold_;

  // JetBrains Mono for numerical values
  juce::Typeface::Ptr jetBrainsMono_;

  // Inter legacy typefaces
  juce::Typeface::Ptr interRegular_;
  juce::Typeface::Ptr interMedium_;
  juce::Typeface::Ptr interBold_;

  // Non-copyable
  Typography(const Typography &) = delete;
  Typography &operator=(const Typography &) = delete;
};

} // namespace uds
