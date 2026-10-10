#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace
{
    using namespace mb11::ui;

    constexpr int rowTop = 112;
    constexpr int rowSpacing = 30;
    constexpr int ledSize = 16;

    constexpr int masterLedX = 160;
    constexpr int soundLedX = 646;

    const char* const soundLabels[]  = { "Tune", "Pitch", "Decay", "Harmonics", "Pulse", "Noise", "Attack", "EQ" };
    const char* const redLabels[]    = { "Volume", "Gate", "Compr", "MetNze" };
    const char* const masterLabels[] = { "Midi Ch", "Split Mode", "Pitch Mode", "LFO Wave", "LFO Speed", "LFO Int", "LFO Sync", "Store" };
    const char* const secondLabels[] = { "X Trig Level", "X Trig On/Off", "Sys Dmp", "Sys Load", "LFO One Shot" };

    juce::Rectangle<int> ledBounds (int x, int row)
    {
        return { x, rowTop + row * rowSpacing - ledSize / 2, ledSize, ledSize };
    }

    void drawLabel (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                    juce::Colour colour, float height, juce::Justification justification, bool bold = false)
    {
        g.setColour (colour);
        g.setFont (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
        g.drawText (text, area, justification, false);
    }
}

//==============================================================================
FrontPanel::FrontPanel (MB11AudioProcessor& p)
    : processor (p), model (p),
      // in a DAW the keys (Space!) belong to the host
      keyboardControl (p.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
{
    setWantsKeyboardFocus (keyboardControl);

    for (auto* c : std::initializer_list<juce::Component*> { &display, &pot, &encoder, &playButton, &upButton, &downButton,
                                                             &triggerLed, &xTriggerLed, &midiLed })
        addAndMakeVisible (c);

    for (auto& led : soundLeds)  addAndMakeVisible (led);
    for (auto& led : masterLeds) addAndMakeVisible (led);

    auto isCombo = [] (const juce::ModifierKeys& mods) { return mods.isShiftDown() || mods.isPopupMenu(); };

    upButton.onPress   = [this, isCombo] (const juce::ModifierKeys& m) { isCombo (m) ? model.upDown() : model.up(); };
    downButton.onPress = [this, isCombo] (const juce::ModifierKeys& m) { isCombo (m) ? model.upDown() : model.down(); };
    playButton.onPress = [this, isCombo] (const juce::ModifierKeys& m) { isCombo (m) ? model.upPlay() : model.play(); };

    encoder.onTurn  = [this] (int steps) { model.encoderTurn (steps); };
    encoder.onClick = [this] { model.encoderClick(); };
    pot.onMove      = [this] (float position) { model.potMoved (position); };

    model.onSysexDump = [this] { dumpSysex(); };
    model.onSysexLoad = [this] { loadSysex(); };

    upButton.setTooltip ("UP  (Shift-Klick: Up+Down = Master)");
    downButton.setTooltip ("DOWN  (Shift-Klick: Up+Down = Master)");
    playButton.setTooltip ("PLAY  (Shift-Klick: Up+Play = 2nd Function)");
    encoder.setTooltip ("VALUE  (ziehen/scrollen, Klick = Load/Enter)");
    pot.setTooltip ("VALUE Poti (holt den Wert erst ab)");

    setSize (designWidth, designHeight);
    timerCallback();
    startTimerHz (30);
}

FrontPanel::~FrontPanel()
{
    stopTimer();
}

// Standalone keys: Space = Play, Up/Down arrows = Up/Down, Left/Right = encoder,
// Return = encoder click; with Shift: Up+Down (arrows) and Up+Play (Space).
bool FrontPanel::keyPressed (const juce::KeyPress& key)
{
    if (! keyboardControl)
        return false;

    const auto shift = key.getModifiers().isShiftDown();
    const auto code = key.getKeyCode();

    if (code == juce::KeyPress::spaceKey)           shift ? model.upPlay() : model.play();
    else if (code == juce::KeyPress::upKey)         shift ? model.upDown() : model.up();
    else if (code == juce::KeyPress::downKey)       shift ? model.upDown() : model.down();
    else if (code == juce::KeyPress::rightKey)      model.encoderTurn (shift ? 10 : 1);
    else if (code == juce::KeyPress::leftKey)       model.encoderTurn (shift ? -10 : -1);
    else if (code == juce::KeyPress::returnKey)     model.encoderClick();
    else
        return false;

    return true;
}

void FrontPanel::resized()
{
    pot.setBounds (332, 52, 96, 96);
    display.setBounds (303, 168, 154, 66);

    triggerLed.setBounds (323, 248, ledSize, ledSize);
    xTriggerLed.setBounds (372, 248, ledSize, ledSize);
    midiLed.setBounds (421, 248, ledSize, ledSize);

    encoder.setBounds (340, 296, 80, 80);
    playButton.setBounds (262, 312, 46, 46);
    upButton.setBounds (458, 296, 38, 38);
    downButton.setBounds (458, 342, 38, 38);

    for (int i = 0; i < 8; ++i)
    {
        soundLeds[(size_t) i].setBounds (ledBounds (soundLedX, i));
        masterLeds[(size_t) i].setBounds (ledBounds (masterLedX, i));
    }
}

void FrontPanel::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (colours::panelLight, 0.0f, 0.0f, colours::panel, 0.0f, bounds.getBottom(), false));
    g.fillAll();

    // brushed metal hint
    g.setColour (juce::Colours::white.withAlpha (0.015f));
    for (int y = 0; y < getHeight(); y += 3)
        g.drawHorizontalLine (y, 0.0f, bounds.getRight());

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRect (getLocalBounds(), 2);

    // logo
    drawLabel (g, "MB-11", { 28, 16, 200, 40 }, colours::label, 34.0f, juce::Justification::left, true);
    drawLabel (g, "ANALOG BASS DRUM", { 30, 54, 200, 16 }, colours::labelDim, 12.0f, juce::Justification::left);

    // master column
    drawLabel (g, "MASTER  (UP+DOWN)", { 10, 80, 140, 16 }, colours::labelDim, 11.0f, juce::Justification::right, true);
    drawLabel (g, "2ND  (UP+PLAY)", { 186, 80, 140, 16 }, colours::labelDim, 11.0f, juce::Justification::left, true);

    for (int i = 0; i < 8; ++i)
    {
        const auto y = rowTop + i * rowSpacing - 9;
        drawLabel (g, masterLabels[i], { 30, y, 120, 18 }, colours::label, 14.0f, juce::Justification::right);

        if (i < 5)
            drawLabel (g, secondLabels[i], { 186, y, 130, 18 }, colours::labelDim, 13.0f, juce::Justification::left);
    }

    // sound column: red parameters left, black (here: light) parameters right
    for (int i = 0; i < 8; ++i)
    {
        const auto y = rowTop + i * rowSpacing - 9;

        if (i < 4)
            drawLabel (g, redLabels[i], { 530, y, 106, 18 }, colours::red, 14.0f, juce::Justification::right);

        drawLabel (g, soundLabels[i], { 672, y, 86, 18 }, colours::label, 14.0f, juce::Justification::left);
    }

    // centre section
    drawLabel (g, "VALUE", { 330, 34, 100, 16 }, colours::labelDim, 11.0f, juce::Justification::centred, true);
    drawLabel (g, "Trig",  { 306, 266, 50, 14 }, colours::labelDim, 11.0f, juce::Justification::centred);
    drawLabel (g, "Xtrig", { 355, 266, 50, 14 }, colours::labelDim, 11.0f, juce::Justification::centred);
    drawLabel (g, "Midi",  { 404, 266, 50, 14 }, colours::labelDim, 11.0f, juce::Justification::centred);

    drawLabel (g, "PLAY", { 255, 360, 60, 14 }, colours::label, 11.0f, juce::Justification::centred, true);
    drawLabel (g, "UP",   { 500, 308, 50, 14 }, colours::label, 11.0f, juce::Justification::left, true);
    drawLabel (g, "DOWN", { 500, 354, 50, 14 }, colours::label, 11.0f, juce::Justification::left, true);
    drawLabel (g, "VALUE  -  Klick = Load/Enter", { 290, 380, 180, 14 }, colours::labelDim, 10.5f, juce::Justification::centred);

    drawLabel (g, "Shift-Klick auf UP/DOWN = Up+Down,  auf PLAY = Up+Play", { 30, 384, 250, 14 },
               colours::labelDim, 10.0f, juce::Justification::left);
}

void FrontPanel::timerCallback()
{
    // The window is not on screen yet when the panel is created, so take the
    // keyboard focus as soon as it is.
    if (keyboardControl && ! hasKeyboardFocus (true) && isShowing()
        && getTopLevelComponent()->hasKeyboardFocus (true) == false && juce::Process::isForegroundProcess())
        grabKeyboardFocus();

    const auto now = juce::Time::getMillisecondCounterHiRes();
    model.tick (now);

    const auto& activity = processor.getActivity();
    auto flash = [now] (const std::atomic<uint32_t>& counter, uint32_t& last, double& until)
    {
        const auto value = counter.load();

        if (value != last)
        {
            last = value;
            until = now + 90.0;
        }

        return now < until;
    };

    const auto triggerOn  = flash (activity.trigger, lastTrigger, triggerUntil);
    const auto xTriggerOn = flash (activity.xTrigger, lastXTrigger, xTriggerUntil);
    const auto midiOn     = flash (activity.midi, lastMidi, midiUntil);

    triggerLed.setOn (triggerOn);
    xTriggerLed.setOn (xTriggerOn);
    midiLed.setOn (midiOn);

    display.setText (model.getDisplayText());
    display.setLit (model.isDisplayVisible());
    display.setDot (0, midiOn);
    display.setDot (1, xTriggerOn);
    display.setDot (2, model.isEditOn());

    const auto blinkPhase = std::fmod (now, 500.0) < 300.0;

    for (int i = 0; i < 8; ++i)
    {
        soundLeds[(size_t) i].setOn (model.getSoundLed() == i && (! model.isSoundLedBlinking() || blinkPhase));
        masterLeds[(size_t) i].setOn (model.getMasterLed() == i && (! model.isMasterLedBlinking() || blinkPhase));
    }

    pot.setPosition (model.getPotPosition());
    pot.setCaught (model.isPotCaught());
}

void FrontPanel::dumpSysex()
{
    const auto slot = mb11::presets::slotLabel (processor.getCurrentPreset());
    fileChooser = std::make_unique<juce::FileChooser> ("Sys Dmp: Preset als SysEx speichern",
                                                      juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                                          .getChildFile ("MB-11 " + slot + ".syx"),
                                                      "*.syx");

    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& chooser)
                              {
                                  const auto file = chooser.getResult();

                                  if (file == juce::File())
                                      return;

                                  const auto data = processor.exportSysex();
                                  model.showMessage (file.replaceWithData (data.getData(), data.getSize()) ? "rdY" : "Err");
                              });
}

void FrontPanel::loadSysex()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Sys Load: SysEx-Preset laden",
                                                      juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                                                      "*.syx");

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& chooser)
                              {
                                  const auto file = chooser.getResult();

                                  if (file == juce::File())
                                      return;

                                  juce::MemoryBlock data;
                                  const auto ok = file.loadFileAsData (data) && processor.importSysex (data);
                                  model.showMessage (ok ? "fin" : "Err");
                              });
}

//==============================================================================
MB11AudioProcessorEditor::MB11AudioProcessorEditor (MB11AudioProcessor& p)
    : AudioProcessorEditor (&p), panel (p)
{
    addAndMakeVisible (panel);

    setResizable (true, true);
    setResizeLimits (FrontPanel::designWidth / 2, FrontPanel::designHeight / 2,
                     FrontPanel::designWidth * 2, FrontPanel::designHeight * 2);
    getConstrainer()->setFixedAspectRatio ((double) FrontPanel::designWidth / FrontPanel::designHeight);
    setSize (FrontPanel::designWidth, FrontPanel::designHeight);
}

void MB11AudioProcessorEditor::resized()
{
    const auto scale = (float) getWidth() / (float) FrontPanel::designWidth;
    panel.setTransform (juce::AffineTransform::scale (scale));
    panel.setBounds (0, 0, FrontPanel::designWidth, FrontPanel::designHeight);
}
