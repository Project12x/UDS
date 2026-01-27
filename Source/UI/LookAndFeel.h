#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace uds {

/**
 * @brief Industrial dark look and feel for UDS
 *
 * Color palette:
 * - Background: #1a1a1a (near black)
 * - Surface: #2a2a2a (dark gray)
 * - Accent: #00b4d8 (cyan)
 * - Accent Dark: #0077b6 (dark blue)
 * - Text: #e0e0e0 (light gray)
 * - Muted: #808080 (medium gray)
 */
class UDSLookAndFeel : public juce::LookAndFeel_V4 {
public:
  // Color constants for reuse
  static constexpr uint32_t kBackground = 0xff1a1a1a;
  static constexpr uint32_t kSurface = 0xff2a2a2a;
  static constexpr uint32_t kSurfaceHover = 0xff3a3a3a;
  static constexpr uint32_t kAccent = 0xff00b4d8;
  static constexpr uint32_t kAccentDark = 0xff0077b6;
  static constexpr uint32_t kText = 0xffe0e0e0;
  static constexpr uint32_t kTextMuted = 0xff808080;
  static constexpr uint32_t kBorder = 0xff505050;

  UDSLookAndFeel() {
    // Dark industrial palette
    setColour(juce::ResizableWindow::backgroundColourId,
              juce::Colour(kBackground));
    setColour(juce::Slider::backgroundColourId, juce::Colour(kSurface));
    setColour(juce::Slider::thumbColourId, juce::Colour(kAccent));
    setColour(juce::Slider::trackColourId, juce::Colour(kAccentDark));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(kAccent));
    setColour(juce::Slider::rotarySliderOutlineColourId,
              juce::Colour(kSurface));
    setColour(juce::Label::textColourId, juce::Colour(kText));

    // Buttons
    setColour(juce::TextButton::buttonColourId, juce::Colour(kSurface));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(kAccent));
    setColour(juce::TextButton::textColourOffId, juce::Colour(kText));
    setColour(juce::TextButton::textColourOnId, juce::Colour(kBackground));

    // ComboBox styling
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(kSurface));
    setColour(juce::ComboBox::textColourId, juce::Colour(kText));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(kBorder));
    setColour(juce::ComboBox::arrowColourId, juce::Colour(kAccent));
    setColour(juce::ComboBox::focusedOutlineColourId, juce::Colour(kAccent));

    // PopupMenu styling
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(kSurface));
    setColour(juce::PopupMenu::textColourId, juce::Colour(kText));
    setColour(juce::PopupMenu::highlightedBackgroundColourId,
              juce::Colour(kAccent));
    setColour(juce::PopupMenu::highlightedTextColourId,
              juce::Colour(kBackground));
  }

  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider &slider) override {
    auto radius = static_cast<float>(juce::jmin(width / 2, height / 2)) - 4.0f;
    auto centreX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    auto centreY = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle =
        rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Background circle with hover effect
    bool isHovered = slider.isMouseOverOrDragging();
    g.setColour(juce::Colour(isHovered ? kSurfaceHover : kSurface));
    g.fillEllipse(rx, ry, rw, rw);

    // Arc track (background)
    juce::Path arcBg;
    arcBg.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f, 0.0f,
                        rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(kBorder));
    g.strokePath(arcBg, juce::PathStrokeType(3.0f));

    // Arc (value)
    juce::Path arc;
    arc.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f, 0.0f,
                      rotaryStartAngle, angle, true);
    g.setColour(juce::Colour(kAccent));
    g.strokePath(arc, juce::PathStrokeType(3.0f));

    // Pointer
    juce::Path pointer;
    auto pointerLength = radius * 0.6f;
    pointer.addRectangle(-2.0f, -pointerLength, 4.0f, pointerLength);
    g.setColour(juce::Colour(0xffffffff));
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(
                            centreX, centreY));
  }

  // Custom ComboBox drawing
  void drawComboBox(juce::Graphics &g, int width, int height, bool isButtonDown,
                    int buttonX, int buttonY, int buttonW, int buttonH,
                    juce::ComboBox &box) override {
    auto cornerSize = 4.0f;
    auto bounds =
        juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);

    // Background
    bool isHovered = box.isMouseOver();
    g.setColour(juce::Colour(isHovered ? kSurfaceHover : kSurface));
    g.fillRoundedRectangle(bounds, cornerSize);

    // Border
    g.setColour(juce::Colour(box.hasKeyboardFocus(true) ? kAccent : kBorder));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

    // Arrow
    auto arrowZone =
        juce::Rectangle<int>(buttonX, buttonY, buttonW, buttonH).toFloat();
    auto arrowSize = juce::jmin(12.0f, arrowZone.getWidth() * 0.5f);

    juce::Path arrow;
    arrow.addTriangle(arrowZone.getCentreX() - arrowSize * 0.5f,
                      arrowZone.getCentreY() - arrowSize * 0.25f,
                      arrowZone.getCentreX() + arrowSize * 0.5f,
                      arrowZone.getCentreY() - arrowSize * 0.25f,
                      arrowZone.getCentreX(),
                      arrowZone.getCentreY() + arrowSize * 0.25f);

    g.setColour(juce::Colour(kAccent));
    g.fillPath(arrow);
  }

  // Tooltip styling
  void drawTooltip(juce::Graphics &g, const juce::String &text, int width,
                   int height) override {
    g.setColour(juce::Colour(kSurface));
    g.fillRoundedRectangle(0.0f, 0.0f, static_cast<float>(width),
                           static_cast<float>(height), 4.0f);

    g.setColour(juce::Colour(kAccent));
    g.drawRoundedRectangle(0.5f, 0.5f, static_cast<float>(width) - 1.0f,
                           static_cast<float>(height) - 1.0f, 4.0f, 1.0f);

    g.setColour(juce::Colour(kText));
    g.setFont(juce::Font(12.0f));
    g.drawText(text, 0, 0, width, height, juce::Justification::centred);
  }
};

} // namespace uds
