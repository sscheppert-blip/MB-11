#pragma once

#include <juce_core/juce_core.h>

#include <functional>

class MB11AudioProcessor;

// Operating logic of the front panel, modelled on the hardware:
//
//  - Up/Down step through preset selection and the 12 sound parameters
//    (Tune .. EQ, then the "red" parameters Volume, Gate, Compr, MetNze).
//  - Up+Down opens the master parameters, Up+Play the 2nd functions.
//    Both return after 10 seconds without encoder input.
//  - The endless encoder changes the value, its click is Load/Enter.
//  - The value pot has to pick up the current value before it changes it.
class FrontPanelModel
{
public:
    enum class Mode { preset, sound, master, second, store };

    static constexpr int numSoundItems = 12;
    static constexpr int numMasterItems = 8;
    static constexpr int numSecondItems = 5;
    static constexpr int masterStoreItem = 7;
    static constexpr int secondDumpItem = 2;
    static constexpr int secondLoadItem = 3;

    explicit FrontPanelModel (MB11AudioProcessor&);

    // panel controls
    void up();
    void down();
    void upDown();            // Up + Down: master parameters
    void upPlay();            // Up + Play: 2nd function
    void play();
    void encoderTurn (int steps);
    void encoderClick();
    void potMoved (float newPosition);   // 0..1

    // call regularly (timeouts, blinking)
    void tick (double nowMs);

    // feedback for the 2nd functions Sys Dmp / Sys Load
    void showMessage (const juce::String& text);

    std::function<void()> onSysexDump, onSysexLoad;

    // view state
    Mode getMode() const noexcept   { return mode; }
    int getItem() const noexcept    { return item; }
    juce::String getDisplayText() const;
    bool isDisplayVisible() const;           // blinks while storing
    bool isEditOn() const;

    int getSoundLed() const noexcept;        // -1 = none
    bool isSoundLedBlinking() const noexcept;
    int getMasterLed() const noexcept;       // -1 = none
    bool isMasterLedBlinking() const noexcept;

    float getPotPosition() const noexcept   { return potPosition; }
    bool isPotCaught() const noexcept       { return potCaught; }

    static const char* getSoundParamId (int index);
    static const char* getMasterParamId (int index);   // nullptr for Store
    static const char* getSecondParamId (int index);   // nullptr for Sys Dmp / Sys Load

private:
    const char* currentParamId() const;
    juce::String formatValue (const char* paramId) const;
    void changeCurrentValue (int steps);
    void setMode (Mode newMode, int newItem);
    void enterTimedMode (Mode newMode);
    void leaveTimedMode();
    void touch();

    MB11AudioProcessor& processor;
    Mode mode = Mode::preset;
    int item = 0;

    Mode returnMode = Mode::preset;
    int returnItem = 0;
    int storeSlot = 0;

    double now = 0.0, lastInput = 0.0;
    juce::String message;
    double messageUntil = 0.0;

    float potPosition = 0.5f;
    bool potCaught = false;
};
