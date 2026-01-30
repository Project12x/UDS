#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>


namespace uds {

/**
 * @brief Slider with right-click context menu for expression pedal assignment
 */
class ExpressionSlider : public juce::Slider {
public:
  // Callback when user assigns expression to this slider
  std::function<void(const juce::String& paramId)> onAssignExpression;
  std::function<void()> onClearExpression;
  std::function<void(const juce::String& paramId)> onTrainRange;
  std::function<bool()>
      isExpressionAssigned; // Returns true if this param has expression

  ExpressionSlider() = default;
  explicit ExpressionSlider(const juce::String& paramId) : paramId_(paramId) {}

  void setParameterId(const juce::String& paramId) { paramId_ = paramId; }
  const juce::String& getParameterId() const { return paramId_; }

  void mouseDown(const juce::MouseEvent& e) override {
    if (e.mods.isPopupMenu()) {
      showExpressionMenu();
    } else {
      Slider::mouseDown(e);
    }
  }

private:
  juce::String paramId_;

  void showExpressionMenu() {
    juce::PopupMenu menu;

    bool hasExpression = isExpressionAssigned && isExpressionAssigned();

    if (hasExpression) {
      menu.addItem(1, "Clear Expression Assignment");
      menu.addItem(2, "Train Expression Range...");
    } else {
      menu.addItem(3, "Assign Expression Pedal");
    }

    menu.showMenuAsync(juce::PopupMenu::Options(),
                       [this, hasExpression](int result) {
                         if (result == 1 && onClearExpression) {
                           onClearExpression();
                         } else if (result == 2 && onTrainRange) {
                           onTrainRange(paramId_);
                         } else if (result == 3 && onAssignExpression) {
                           onAssignExpression(paramId_);
                         }
                       });
  }
};

} // namespace uds
