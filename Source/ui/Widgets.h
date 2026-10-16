#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace mb11::ui
{
namespace colours
{
    inline const juce::Colour panel      { 0xff2b2d30 };
    inline const juce::Colour panelLight { 0xff383b3f };
    inline const juce::Colour label      { 0xffd9d9d4 };
    inline const juce::Colour labelDim   { 0xff8d9094 };
    inline const juce::Colour red        { 0xffe8473b };
    inline const juce::Colour ledOn      { 0xffff3b2a };
    inline const juce::Colour ledOff     { 0xff4a1d1a };
}

// Three-digit red seven-segment LED display with decimal points.
class SevenSegmentDisplay final : public juce::Component
{
public:
    void setText (const juce::String& text);
    void setDot (int digit, bool on);
    void setLit (bool shouldBeLit);

    void paint (juce::Graphics&) override;

private:
    static uint8_t segmentsFor (juce::juce_wchar);
    void paintDigit (juce::Graphics&, juce::Rectangle<float>, uint8_t segments, bool dot) const;

    juce::String text { "   " };
    std::array<bool, 3> dots {};
    bool lit = true;
};

class Led final : public juce::Component
{
public:
    void setOn (bool shouldBeOn);
    void paint (juce::Graphics&) override;

private:
    bool on = false;
};

// Round push button; reports the modifier keys so that Shift-click can stand
// in for pressing two buttons at once.
class PanelButton final : public juce::Component,
                          public juce::SettableTooltipClient
{
public:
    std::function<void (const juce::ModifierKeys&)> onPress;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    bool down = false;
};

// Endless rotary encoder with push (click = Load/Enter).
class Encoder final : public juce::Component,
                      public juce::SettableTooltipClient
{
public:
    std::function<void (int)> onTurn;
    std::function<void()> onClick;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void turn (int steps);

    float angle = 0.0f;
    float dragAccumulator = 0.0f, wheelAccumulator = 0.0f;
    int lastDragY = 0;
    bool dragged = false, pressed = false;
};

// Value potentiometer with an absolute position.
class ValuePot final : public juce::Component,
                       public juce::SettableTooltipClient
{
public:
    std::function<void (float)> onMove;

    void setPosition (float newPosition);
    void setCaught (bool isCaught);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void moveTo (float newPosition);

    float position = 0.5f, dragStartPosition = 0.5f;
    bool caught = false;
};

// Invisible click area, e.g. over the logo.
class ClickArea final : public juce::Component,
                        public juce::SettableTooltipClient
{
public:
    ClickArea();

    std::function<void()> onClick;

    void mouseUp (const juce::MouseEvent&) override;
};

// Version popup in the style of the panel; a click anywhere closes it.
class InfoPopup final : public juce::Component
{
public:
    struct Row { juce::String label, value; };

    std::function<void()> onClose;

    void setRows (std::vector<Row> newRows);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    std::vector<Row> rows;
};
}
