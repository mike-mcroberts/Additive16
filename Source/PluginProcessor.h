#pragma once
#include <JuceHeader.h>
#include <array>

class Additive16AudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int partialCount = 16;
    static constexpr int voiceCount = 12;
    Additive16AudioProcessor();
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Additive 16"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 15.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Init"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    static juce::String id(int partial, const char* suffix);
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();

private:
    struct Envelope
    {
        enum Stage { off, attack, decay, sustain, release } stage = off;
        double value = 0.0, releaseStep = 0.0;
        void start() { value = 0.0; stage = attack; }
        void stop(double releaseSeconds, double sampleRate)
        {
            if (stage == off) return;
            stage = release;
            releaseStep = value / juce::jmax(1.0, releaseSeconds * sampleRate);
        }
        double next(double a, double d, double s, double sampleRate)
        {
            switch (stage)
            {
                case off: return 0.0;
                case attack:
                    value += 1.0 / juce::jmax(1.0, a * sampleRate);
                    if (value >= 1.0) { value = 1.0; stage = decay; }
                    break;
                case decay:
                    value -= (1.0 - s) / juce::jmax(1.0, d * sampleRate);
                    if (value <= s) { value = s; stage = sustain; }
                    break;
                case sustain: value = s; break;
                case release:
                    value -= releaseStep;
                    if (value <= 0.0) { value = 0.0; stage = off; }
                    break;
            }
            return value;
        }
    };
    struct Voice
    {
        int note = -1;
        int channel = 1;
        double velocity = 0.0;
        uint64_t age = 0;
        std::array<double, partialCount> phase {};
        std::array<Envelope, partialCount> envelopes {};
        bool active() const
        {
            for (const auto& env : envelopes) if (env.stage != Envelope::off) return true;
            return false;
        }
    };
    struct PartialParams
    {
        std::atomic<float>* level = nullptr;
        std::atomic<float>* ratio = nullptr;
        std::atomic<float>* hz = nullptr;
        std::atomic<float>* mode = nullptr;
        std::atomic<float>* attack = nullptr;
        std::atomic<float>* decay = nullptr;
        std::atomic<float>* sustain = nullptr;
        std::atomic<float>* release = nullptr;
    };
    std::array<PartialParams, partialCount> cached {};
    std::array<Voice, voiceCount> voices {};
    std::atomic<float>* master = nullptr;
    double currentSampleRate = 44100.0;
    uint64_t voiceAge = 0;
    void noteOn(int note, int channel, float velocity);
    void noteOff(int note, int channel);
    void allNotesOff();
    void render(juce::AudioBuffer<float>&, int start, int length);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Additive16AudioProcessor)
};
