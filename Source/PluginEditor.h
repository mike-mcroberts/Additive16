#pragma once
#include "PluginProcessor.h"
#include <array>

class Additive16AudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit Additive16AudioProcessorEditor(Additive16AudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };
    Additive16AudioProcessor& processor;
    std::array<juce::TextButton, 16> partialButtons;
    std::array<juce::Label, 16> partialSummaries;
    std::array<Knob, 7> knobs;
    juce::ComboBox pitchMode;
    juce::Label pitchModeLabel;
    std::unique_ptr<ComboAttachment> pitchAttachment;
    Knob master;
    int selected = 0;
    void selectPartial(int index);
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Additive16AudioProcessorEditor)
};
