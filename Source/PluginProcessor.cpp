#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

juce::String Additive16AudioProcessor::id(int p, const char* suffix)
{
    return "p" + juce::String(p + 1) + "_" + suffix;
}

juce::AudioProcessorValueTreeState::ParameterLayout Additive16AudioProcessor::makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto addFloat = [&layout](const juce::String& key, const juce::String& label,
                              float min, float max, float def, float skew = 1.0f)
    {
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { key, 1 }, label,
            juce::NormalisableRange<float>(min, max, 0.0f, skew), def));
    };
    addFloat("master", "Master level", 0.0f, 1.0f, 0.35f);
    for (int p = 0; p < partialCount; ++p)
    {
        const auto name = "Partial " + juce::String(p + 1) + " ";
        addFloat(id(p, "level"), name + "Level", 0.0f, 1.0f, p == 0 ? 0.8f : 0.25f / (p + 1));
        addFloat(id(p, "ratio"), name + "Ratio", 0.01f, 64.0f, float(p + 1));
        addFloat(id(p, "hz"), name + "Absolute Hz", 20.0f, 20000.0f, juce::jmin(20000.0f, 220.0f * float(p + 1)), 0.35f);
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { id(p, "mode"), 1 }, name + "Pitch mode",
            juce::StringArray { "Ratio", "Absolute Hz" }, 0));
        addFloat(id(p, "attack"), name + "Attack s", 0.001f, 10.0f, 0.01f, 0.4f);
        addFloat(id(p, "decay"), name + "Decay s", 0.001f, 10.0f, 0.2f, 0.4f);
        addFloat(id(p, "sustain"), name + "Sustain", 0.0f, 1.0f, 0.7f);
        addFloat(id(p, "release"), name + "Release s", 0.001f, 15.0f, 0.35f, 0.4f);
    }
    return layout;
}

Additive16AudioProcessor::Additive16AudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", makeLayout())
{
    master = parameters.getRawParameterValue("master");
    for (int p = 0; p < partialCount; ++p)
    {
        auto& c = cached[size_t(p)];
        c.level = parameters.getRawParameterValue(id(p, "level"));
        c.ratio = parameters.getRawParameterValue(id(p, "ratio"));
        c.hz = parameters.getRawParameterValue(id(p, "hz"));
        c.mode = parameters.getRawParameterValue(id(p, "mode"));
        c.attack = parameters.getRawParameterValue(id(p, "attack"));
        c.decay = parameters.getRawParameterValue(id(p, "decay"));
        c.sustain = parameters.getRawParameterValue(id(p, "sustain"));
        c.release = parameters.getRawParameterValue(id(p, "release"));
    }
}

void Additive16AudioProcessor::prepareToPlay(double sr, int)
{
    currentSampleRate = sr;
    allNotesOff();
}

bool Additive16AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void Additive16AudioProcessor::noteOn(int note, int channel, float velocity)
{
    Voice* selected = nullptr;
    for (auto& voice : voices)
        if (!voice.active()) { selected = &voice; break; }
    if (selected == nullptr)
    {
        selected = &voices[0];
        for (auto& voice : voices)
            if (voice.age < selected->age) selected = &voice;
    }
    selected->note = note;
    selected->channel = channel;
    selected->velocity = velocity;
    selected->age = ++voiceAge;
    selected->phase.fill(0.0);
    for (auto& env : selected->envelopes) env.start();
}

void Additive16AudioProcessor::noteOff(int note, int channel)
{
    for (auto& voice : voices)
        if (voice.note == note && voice.channel == channel && voice.active())
            for (int p = 0; p < partialCount; ++p)
                voice.envelopes[size_t(p)].stop(cached[size_t(p)].release->load(), currentSampleRate);
}

void Additive16AudioProcessor::allNotesOff()
{
    for (auto& voice : voices) voice = Voice {};
}

void Additive16AudioProcessor::render(juce::AudioBuffer<float>& buffer, int start, int length)
{
    struct Snapshot { double level, ratio, hz, a, d, s; bool absolute; };
    std::array<Snapshot, partialCount> params {};
    for (int p = 0; p < partialCount; ++p)
    {
        const auto& c = cached[size_t(p)];
        params[size_t(p)] = { c.level->load(), c.ratio->load(), c.hz->load(),
            c.attack->load(), c.decay->load(), c.sustain->load(), c.mode->load() >= 0.5f };
    }
    const double gain = master->load() / 4.0;
    const double twoPi = juce::MathConstants<double>::twoPi;
    const double maxFreq = 0.49 * currentSampleRate;
    for (auto& voice : voices)
    {
        if (!voice.active()) continue;
        const double fundamental = 440.0 * std::pow(2.0, (voice.note - 69) / 12.0);
        for (int i = start; i < start + length; ++i)
        {
            double sample = 0.0;
            for (int p = 0; p < partialCount; ++p)
            {
                const auto& prm = params[size_t(p)];
                auto& env = voice.envelopes[size_t(p)];
                if (env.stage == Envelope::off) continue;
                const double amplitude = env.next(prm.a, prm.d, prm.s, currentSampleRate);
                const double freq = prm.absolute ? prm.hz : fundamental * prm.ratio;
                if (freq > 0.0 && freq < maxFreq)
                {
                    sample += std::sin(voice.phase[size_t(p)]) * amplitude * prm.level;
                    voice.phase[size_t(p)] += twoPi * freq / currentSampleRate;
                    if (voice.phase[size_t(p)] >= twoPi) voice.phase[size_t(p)] -= twoPi;
                }
            }
            sample = std::tanh(sample * gain * voice.velocity);
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.addSample(ch, i, float(sample));
        }
    }
}

void Additive16AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    int cursor = 0;
    for (const auto metadata : midi)
    {
        const int pos = juce::jlimit(0, buffer.getNumSamples(), metadata.samplePosition);
        if (pos > cursor) render(buffer, cursor, pos - cursor);
        cursor = pos;
        const auto message = metadata.getMessage();
        if (message.isNoteOn()) noteOn(message.getNoteNumber(), message.getChannel(), message.getFloatVelocity());
        else if (message.isNoteOff()) noteOff(message.getNoteNumber(), message.getChannel());
        else if (message.isAllNotesOff() || message.isAllSoundOff()) allNotesOff();
    }
    if (cursor < buffer.getNumSamples()) render(buffer, cursor, buffer.getNumSamples() - cursor);
}

void Additive16AudioProcessor::getStateInformation(juce::MemoryBlock& block)
{
    auto xml = parameters.copyState().createXml();
    copyXmlToBinary(*xml, block);
}

void Additive16AudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* Additive16AudioProcessor::createEditor()
{
    return new Additive16AudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Additive16AudioProcessor();
}
