#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "ui/FrontPanelModel.h"
#include "ui/Widgets.h"

class MB11AudioProcessor;

// The front panel at its design size; the editor scales it.
class FrontPanel final : public juce::Component,
                         private juce::Timer
{
public:
    static constexpr int designWidth = 760;
    static constexpr int designHeight = 410;

    explicit FrontPanel (MB11AudioProcessor&);
    ~FrontPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void dumpSysex();
    void loadSysex();

    MB11AudioProcessor& processor;
    FrontPanelModel model;

    mb11::ui::SevenSegmentDisplay display;
    mb11::ui::ValuePot pot;
    mb11::ui::Encoder encoder;
    mb11::ui::PanelButton playButton, upButton, downButton;
    mb11::ui::Led triggerLed, xTriggerLed, midiLed;
    std::array<mb11::ui::Led, 8> soundLeds, masterLeds;

    uint32_t lastTrigger = 0, lastXTrigger = 0, lastMidi = 0;
    double triggerUntil = 0.0, xTriggerUntil = 0.0, midiUntil = 0.0;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrontPanel)
};

class MB11AudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit MB11AudioProcessorEditor (MB11AudioProcessor&);

    void resized() override;

private:
    FrontPanel panel;
    juce::TooltipWindow tooltipWindow { this, 700 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MB11AudioProcessorEditor)
};
