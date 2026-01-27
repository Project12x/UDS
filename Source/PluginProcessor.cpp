// PluginProcessor.cpp
#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorEditor *UDSAudioProcessor::createEditor() {
  return new UDSAudioProcessorEditor(*this);
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new UDSAudioProcessor();
}
