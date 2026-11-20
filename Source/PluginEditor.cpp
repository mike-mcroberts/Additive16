#include "PluginEditor.h"

Additive16AudioProcessorEditor::Additive16AudioProcessorEditor(Additive16AudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(960, 610);
    for (int i = 0; i < 16; ++i)
    {
        auto& b = partialButtons[size_t(i)];
        b.setButtonText("PARTIAL " + juce::String(i + 1));
        b.onClick = [this, i] { selectPartial(i); };
        addAndMakeVisible(b);
        auto& summary = partialSummaries[size_t(i)];
        summary.setJustificationType(juce::Justification::centred);
        summary.setFont(juce::Font(juce::FontOptions(12.0f)));
        addAndMakeVisible(summary);
    }
    const char* names[] = { "LEVEL", "RATIO", "FREQUENCY Hz", "ATTACK s", "DECAY s", "SUSTAIN", "RELEASE s" };
    for (int i = 0; i < 7; ++i)
    {
        auto& k = knobs[size_t(i)];
        k.label.setText(names[i], juce::dontSendNotification);
        k.label.setJustificationType(juce::Justification::centred);
        k.label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(k.label);
        k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 23);
        addAndMakeVisible(k.slider);
    }
    pitchMode.addItem("Ratio (follows MIDI)", 1);
    pitchMode.addItem("Absolute Hz", 2);
    addAndMakeVisible(pitchMode);
    pitchModeLabel.setText("PITCH MODE", juce::dontSendNotification);
    pitchModeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(pitchModeLabel);
    master.label.setText("MASTER", juce::dontSendNotification);
    master.label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(master.label);
    master.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    master.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 23);
    addAndMakeVisible(master.slider);
    master.attachment = std::make_unique<SliderAttachment>(processor.parameters, "master", master.slider);
    selectPartial(0);
    startTimerHz(8);
}

void Additive16AudioProcessorEditor::selectPartial(int index)
{
    selected = index;
    const char* keys[] = { "level", "ratio", "hz", "attack", "decay", "sustain", "release" };
    for (int i = 0; i < 7; ++i)
    {
        auto& k = knobs[size_t(i)];
        k.attachment.reset();
        k.attachment = std::make_unique<SliderAttachment>(processor.parameters,
            Additive16AudioProcessor::id(selected, keys[i]), k.slider);
    }
    pitchAttachment.reset();
    pitchAttachment = std::make_unique<ComboAttachment>(processor.parameters,
        Additive16AudioProcessor::id(selected, "mode"), pitchMode);
    for (int i = 0; i < 16; ++i)
        partialButtons[size_t(i)].setColour(juce::TextButton::buttonColourId,
            i == selected ? juce::Colour(0xff287c96) : juce::Colour(0xff303b4c));
}

void Additive16AudioProcessorEditor::timerCallback()
{
    for (int i = 0; i < 16; ++i)
    {
        const auto level = processor.parameters.getRawParameterValue(Additive16AudioProcessor::id(i, "level"))->load();
        const auto ratio = processor.parameters.getRawParameterValue(Additive16AudioProcessor::id(i, "ratio"))->load();
        const auto hz = processor.parameters.getRawParameterValue(Additive16AudioProcessor::id(i, "hz"))->load();
        const bool absolute = processor.parameters.getRawParameterValue(Additive16AudioProcessor::id(i, "mode"))->load() >= 0.5f;
        partialSummaries[size_t(i)].setText(
            juce::String(level, 2) + "  |  " + (absolute ? juce::String(hz, 1) + " Hz" : juce::String(ratio, 2) + "x"),
            juce::dontSendNotification);
    }
}

void Additive16AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff151d2a));
    g.setColour(juce::Colour(0xffe6f1fa));
    g.setFont(juce::Font(juce::FontOptions(29.0f, juce::Font::bold)));
    g.drawText("ADDITIVE 16", 28, 16, 360, 42, juce::Justification::centredLeft);
    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    g.setColour(juce::Colour(0xff92acbd));
    g.drawText("16 independent oscillators  |  16 independent ADSR envelopes", 30, 58, 660, 24,
               juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff253348));
    g.fillRoundedRectangle(20.0f, 99.0f, 920.0f, 238.0f, 12.0f);
    g.fillRoundedRectangle(20.0f, 350.0f, 920.0f, 239.0f, 12.0f);
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::bold)));
    g.drawText("OSCILLATORS — select a partial", 34, 108, 550, 23, juce::Justification::left);
    g.drawText("EDIT PARTIAL " + juce::String(selected + 1), 34, 361, 550, 24, juce::Justification::left);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.setColour(juce::Colour(0xffa6bdc9));
    g.drawText("Ratio follows the MIDI note; Absolute Hz ignores MIDI pitch. Double-click a value to type it.",
               34, 560, 865, 22, juce::Justification::left);
}

void Additive16AudioProcessorEditor::resized()
{
    const int x0 = 34, y0 = 143, w = 108, h = 63, dx = 113, dy = 94;
    for (int i = 0; i < 16; ++i)
    {
        const int x = x0 + (i % 8) * dx, y = y0 + (i / 8) * dy;
        partialButtons[size_t(i)].setBounds(x, y, w, 34);
        partialSummaries[size_t(i)].setBounds(x - 2, y + 36, w + 4, 22);
    }
    for (int i = 0; i < 7; ++i)
    {
        const int x = 30 + i * 112;
        knobs[size_t(i)].label.setBounds(x, 407, 110, 22);
        knobs[size_t(i)].slider.setBounds(x, 429, 110, 118);
    }
    pitchModeLabel.setBounds(816, 397, 116, 22);
    pitchMode.setBounds(806, 423, 126, 30);
    master.label.setBounds(818, 463, 110, 20);
    master.slider.setBounds(820, 484, 106, 71);
}
