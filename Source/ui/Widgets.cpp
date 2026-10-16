#include "Widgets.h"

namespace mb11::ui
{
namespace
{
    // segment bits: a = top, b = upper right, c = lower right, d = bottom,
    // e = lower left, f = upper left, g = middle
    enum : uint8_t { a = 1, b = 2, c = 4, d = 8, e = 16, f = 32, g = 64 };

    // Wheel movement; with Shift macOS reports a vertical wheel as horizontal.
    float wheelDelta (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
    {
        auto delta = wheel.deltaY;

        if (event.mods.isShiftDown() && juce::approximatelyEqual (delta, 0.0f))
            delta = wheel.deltaX;

        return wheel.isReversed ? -delta : delta;
    }

    void paintKnob (juce::Graphics& gr, juce::Rectangle<float> area, float angle, juce::Colour pointer)
    {
        const auto size = juce::jmin (area.getWidth(), area.getHeight());
        const auto knob = area.withSizeKeepingCentre (size, size).reduced (2.0f);

        gr.setColour (juce::Colours::black.withAlpha (0.45f));
        gr.fillEllipse (knob.translated (0.0f, 3.0f));

        juce::ColourGradient body (juce::Colour (0xff5a5d62), knob.getCentreX(), knob.getY(),
                                   juce::Colour (0xff1c1d1f), knob.getCentreX(), knob.getBottom(), false);
        gr.setGradientFill (body);
        gr.fillEllipse (knob);

        const auto cap = knob.reduced (size * 0.12f);
        juce::ColourGradient capFill (juce::Colour (0xff45484c), cap.getX(), cap.getY(),
                                      juce::Colour (0xff222326), cap.getRight(), cap.getBottom(), false);
        gr.setGradientFill (capFill);
        gr.fillEllipse (cap);

        gr.setColour (juce::Colours::black.withAlpha (0.6f));
        gr.drawEllipse (knob, 1.0f);

        const auto centre = knob.getCentre();
        const auto radius = cap.getWidth() * 0.5f;
        juce::Line<float> line (centre.getPointOnCircumference (radius * 0.35f, angle),
                                centre.getPointOnCircumference (radius * 0.92f, angle));
        gr.setColour (pointer);
        gr.drawLine (line, juce::jmax (2.0f, size * 0.045f));
    }
}

//==============================================================================
void SevenSegmentDisplay::setText (const juce::String& newText)
{
    const auto padded = newText.substring (0, 3).paddedRight (' ', 3);

    if (padded != text)
    {
        text = padded;
        repaint();
    }
}

void SevenSegmentDisplay::setDot (int digit, bool on)
{
    if (dots[(size_t) digit] != on)
    {
        dots[(size_t) digit] = on;
        repaint();
    }
}

void SevenSegmentDisplay::setLit (bool shouldBeLit)
{
    if (lit != shouldBeLit)
    {
        lit = shouldBeLit;
        repaint();
    }
}

uint8_t SevenSegmentDisplay::segmentsFor (juce::juce_wchar ch)
{
    switch (ch)
    {
        case '0': case 'O': return a | b | c | d | e | f;
        case '1': return b | c;
        case '2': return a | b | g | e | d;
        case '3': return a | b | g | c | d;
        case '4': return f | g | b | c;
        case '5': case 'S': case 's': return a | f | g | c | d;
        case '6': return a | f | g | e | d | c;
        case '7': return a | b | c;
        case '8': return a | b | c | d | e | f | g;
        case '9': return a | b | c | d | f | g;
        case 'A': return a | b | c | e | f | g;
        case 'b': return c | d | e | f | g;
        case 'C': return a | d | e | f;
        case 'c': return d | e | g;
        case 'd': return b | c | d | e | g;
        case 'E': return a | d | e | f | g;
        case 'e': return a | b | d | e | f | g;
        case 'F': case 'f': return a | e | f | g;
        case 'H': return b | c | e | f | g;
        case 'h': return c | e | f | g;
        case 'i': return c;
        case 'I': return b | c;
        case 'L': return d | e | f;
        case 'l': return e | f;
        case 'M': return a | b | c | e | f;
        case 'n': return c | e | g;
        case 'o': return c | d | e | g;
        case 'P': case 'p': return a | b | e | f | g;
        case 'r': return e | g;
        case 't': return d | e | f | g;
        case 'u': return c | d | e;
        case 'U': return b | c | d | e | f;
        case 'Y': case 'y': return b | c | d | f | g;
        case '-': return g;
        case '_': return d;
        default:  return 0;
    }
}

void SevenSegmentDisplay::paintDigit (juce::Graphics& gr, juce::Rectangle<float> area, uint8_t segments, bool dot) const
{
    const auto w = area.getWidth() * 0.78f;
    const auto h = area.getHeight();
    const auto t = w * 0.17f;            // segment thickness
    const auto x0 = area.getX();
    const auto y0 = area.getY();
    const auto skew = 0.08f;             // italic slant like a real LED display

    auto point = [&] (float x, float y)
    {
        return juce::Point<float> (x0 + x + (h - y) * skew, y0 + y);
    };

    auto horizontal = [&] (float y)
    {
        juce::Path p;
        p.startNewSubPath (point (t * 0.6f, y));
        p.lineTo (point (t * 1.1f, y - t * 0.5f));
        p.lineTo (point (w - t * 1.1f, y - t * 0.5f));
        p.lineTo (point (w - t * 0.6f, y));
        p.lineTo (point (w - t * 1.1f, y + t * 0.5f));
        p.lineTo (point (t * 1.1f, y + t * 0.5f));
        p.closeSubPath();
        return p;
    };

    auto vertical = [&] (float x, float yTop, float yBottom)
    {
        juce::Path p;
        p.startNewSubPath (point (x, yTop + t * 0.6f));
        p.lineTo (point (x + t * 0.5f, yTop + t * 1.1f));
        p.lineTo (point (x + t * 0.5f, yBottom - t * 1.1f));
        p.lineTo (point (x, yBottom - t * 0.6f));
        p.lineTo (point (x - t * 0.5f, yBottom - t * 1.1f));
        p.lineTo (point (x - t * 0.5f, yTop + t * 1.1f));
        p.closeSubPath();
        return p;
    };

    const auto top = t * 0.5f, mid = h * 0.5f, bottom = h - t * 0.5f;
    const auto left = t * 0.5f, right = w - t * 0.5f;

    const juce::Path paths[] =
    {
        horizontal (top),                // a
        vertical (right, top, mid),      // b
        vertical (right, mid, bottom),   // c
        horizontal (bottom),             // d
        vertical (left, mid, bottom),    // e
        vertical (left, top, mid),       // f
        horizontal (mid),                // g
    };

    const auto onColour = colours::ledOn;
    const auto offColour = juce::Colour (0xff3a0f0c);

    for (int i = 0; i < 7; ++i)
    {
        const auto on = lit && (segments & (1 << i)) != 0;

        if (on)
        {
            gr.setColour (onColour.withAlpha (0.25f));
            gr.strokePath (paths[i], juce::PathStrokeType (t * 0.6f));
        }

        gr.setColour (on ? onColour : offColour);
        gr.fillPath (paths[i]);
    }

    const auto dotSize = t * 1.1f;
    const auto dotCentre = point (w + t * 0.7f, bottom);
    gr.setColour (lit && dot ? onColour : offColour);
    gr.fillEllipse (juce::Rectangle<float> (dotSize, dotSize).withCentre (dotCentre));
}

void SevenSegmentDisplay::paint (juce::Graphics& gr)
{
    const auto bounds = getLocalBounds().toFloat();

    gr.setColour (juce::Colour (0xff120606));
    gr.fillRoundedRectangle (bounds, 4.0f);
    gr.setColour (juce::Colours::black);
    gr.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.5f);

    auto inner = bounds.reduced (bounds.getHeight() * 0.18f, bounds.getHeight() * 0.16f);
    const auto digitWidth = inner.getWidth() / 3.0f;

    for (int i = 0; i < 3; ++i)
        paintDigit (gr, inner.removeFromLeft (digitWidth).reduced (digitWidth * 0.06f, 0.0f),
                    segmentsFor (text[i]), dots[(size_t) i]);

    // glass reflection
    gr.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.06f), 0.0f, bounds.getY(),
                                              juce::Colours::transparentWhite, 0.0f, bounds.getCentreY(), false));
    gr.fillRoundedRectangle (bounds.reduced (2.0f), 3.0f);
}

//==============================================================================
void Led::setOn (bool shouldBeOn)
{
    if (on != shouldBeOn)
    {
        on = shouldBeOn;
        repaint();
    }
}

void Led::paint (juce::Graphics& gr)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto led = bounds.withSizeKeepingCentre (size, size).reduced (size * 0.2f);

    if (on)
    {
        gr.setColour (colours::ledOn.withAlpha (0.3f));
        gr.fillEllipse (led.expanded (size * 0.18f));
    }

    juce::ColourGradient fill (on ? juce::Colour (0xffffb3a6) : juce::Colour (0xff6a2a25),
                               led.getCentreX() - led.getWidth() * 0.2f, led.getCentreY() - led.getHeight() * 0.2f,
                               on ? colours::ledOn : colours::ledOff, led.getCentreX(), led.getBottom(), true);
    gr.setGradientFill (fill);
    gr.fillEllipse (led);
    gr.setColour (juce::Colours::black.withAlpha (0.7f));
    gr.drawEllipse (led, 1.0f);
}

//==============================================================================
void PanelButton::paint (juce::Graphics& gr)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto cap = bounds.withSizeKeepingCentre (size, size).reduced (2.0f);

    gr.setColour (juce::Colours::black.withAlpha (0.5f));
    gr.fillEllipse (cap.translated (0.0f, down ? 1.0f : 3.0f));

    juce::ColourGradient fill (down ? juce::Colour (0xff2a2c2f) : juce::Colour (0xff6b6f75), cap.getCentreX(), cap.getY(),
                               down ? juce::Colour (0xff3d4044) : juce::Colour (0xff2c2e31), cap.getCentreX(), cap.getBottom(), false);
    gr.setGradientFill (fill);
    gr.fillEllipse (cap.translated (0.0f, down ? 1.0f : 0.0f));
    gr.setColour (juce::Colours::black.withAlpha (0.7f));
    gr.drawEllipse (cap.translated (0.0f, down ? 1.0f : 0.0f), 1.0f);
}

void PanelButton::mouseDown (const juce::MouseEvent& event)
{
    down = true;
    repaint();

    if (onPress != nullptr)
        onPress (event.mods);
}

void PanelButton::mouseUp (const juce::MouseEvent&)
{
    down = false;
    repaint();
}

//==============================================================================
void Encoder::turn (int steps)
{
    angle += (float) steps * juce::MathConstants<float>::twoPi / 24.0f;
    repaint();

    if (onTurn != nullptr)
        onTurn (steps);
}

void Encoder::paint (juce::Graphics& gr)
{
    paintKnob (gr, getLocalBounds().toFloat(), angle, pressed ? colours::red : colours::label);
}

void Encoder::mouseDown (const juce::MouseEvent& event)
{
    lastDragY = event.getPosition().y;
    dragAccumulator = 0.0f;
    dragged = false;
    pressed = true;
    repaint();
}

void Encoder::mouseDrag (const juce::MouseEvent& event)
{
    const auto y = event.getPosition().y;
    const auto pixelsPerStep = event.mods.isShiftDown() ? 12.0f : 4.0f;
    dragAccumulator += (float) (lastDragY - y) / pixelsPerStep;
    lastDragY = y;

    if (std::abs (event.getDistanceFromDragStartY()) > 2)
        dragged = true;

    const auto steps = (int) dragAccumulator;

    if (steps != 0)
    {
        dragAccumulator -= (float) steps;
        turn (steps);
    }
}

void Encoder::mouseUp (const juce::MouseEvent&)
{
    pressed = false;
    repaint();

    if (! dragged && onClick != nullptr)
        onClick();
}

void Encoder::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    // Shift: three times finer, like dragging
    wheelAccumulator += wheelDelta (event, wheel) * (event.mods.isShiftDown() ? 4.0f : 12.0f);
    const auto steps = (int) wheelAccumulator;

    if (steps != 0)
    {
        wheelAccumulator -= (float) steps;
        turn (steps);
    }
}

//==============================================================================
void ValuePot::setPosition (float newPosition)
{
    if (! juce::approximatelyEqual (position, newPosition))
    {
        position = newPosition;
        repaint();
    }
}

void ValuePot::setCaught (bool isCaught)
{
    if (caught != isCaught)
    {
        caught = isCaught;
        repaint();
    }
}

void ValuePot::paint (juce::Graphics& gr)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto startAngle = -0.75f * juce::MathConstants<float>::pi;
    const auto endAngle = 0.75f * juce::MathConstants<float>::pi;
    const auto angle = startAngle + position * (endAngle - startAngle);

    // scale
    const auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto centre = bounds.getCentre();

    gr.setColour (colours::labelDim);

    for (int i = 0; i <= 10; ++i)
    {
        const auto a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
        gr.drawLine (juce::Line<float> (centre.getPointOnCircumference (size * 0.44f, a),
                                        centre.getPointOnCircumference (size * 0.49f, a)), 1.2f);
    }

    paintKnob (gr, bounds.reduced (size * 0.1f), angle, caught ? colours::red : colours::label);
}

void ValuePot::moveTo (float newPosition)
{
    newPosition = juce::jlimit (0.0f, 1.0f, newPosition);

    if (onMove != nullptr)
        onMove (newPosition);

    setPosition (newPosition);
}

void ValuePot::mouseDown (const juce::MouseEvent&)
{
    dragStartPosition = position;
}

void ValuePot::mouseDrag (const juce::MouseEvent& event)
{
    const auto range = event.mods.isShiftDown() ? 800.0f : 200.0f;
    moveTo (dragStartPosition - (float) event.getDistanceFromDragStartY() / range);
}

void ValuePot::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    // Shift: four times finer, like dragging
    moveTo (position + wheelDelta (event, wheel) * (event.mods.isShiftDown() ? 0.025f : 0.1f));
}

//==============================================================================
ClickArea::ClickArea()
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void ClickArea::mouseUp (const juce::MouseEvent& event)
{
    if (onClick != nullptr && getLocalBounds().contains (event.getPosition()))
        onClick();
}

//==============================================================================
void InfoPopup::setRows (std::vector<Row> newRows)
{
    rows = std::move (newRows);
    repaint();
}

void InfoPopup::paint (juce::Graphics& gr)
{
    gr.fillAll (juce::Colours::black.withAlpha (0.55f));

    const auto rowHeight = 22.0f;
    const auto card = getLocalBounds().toFloat().withSizeKeepingCentre (360.0f, 130.0f + rowHeight * (float) rows.size());

    gr.setColour (juce::Colours::black.withAlpha (0.5f));
    gr.fillRoundedRectangle (card.translated (0.0f, 4.0f), 8.0f);

    gr.setGradientFill (juce::ColourGradient (colours::panelLight, 0.0f, card.getY(), colours::panel, 0.0f, card.getBottom(), false));
    gr.fillRoundedRectangle (card, 8.0f);

    // brushed metal hint, like the panel
    {
        juce::Graphics::ScopedSaveState state (gr);
        gr.reduceClipRegion (card.reduced (2.0f).toNearestInt());
        gr.setColour (juce::Colours::white.withAlpha (0.015f));

        for (auto y = card.getY(); y < card.getBottom(); y += 3.0f)
            gr.drawHorizontalLine ((int) y, card.getX(), card.getRight());
    }

    gr.setColour (juce::Colours::black.withAlpha (0.7f));
    gr.drawRoundedRectangle (card, 8.0f, 1.5f);
    gr.setColour (juce::Colours::white.withAlpha (0.06f));
    gr.drawRoundedRectangle (card.reduced (2.0f), 6.5f, 1.0f);

    auto area = card.reduced (26.0f, 18.0f);

    // title with a lit LED
    auto title = area.removeFromTop (38.0f);
    const auto led = title.removeFromRight (14.0f).withSizeKeepingCentre (12.0f, 12.0f);
    gr.setColour (colours::ledOn.withAlpha (0.3f));
    gr.fillEllipse (led.expanded (3.0f));
    gr.setGradientFill (juce::ColourGradient (juce::Colour (0xffffb3a6), led.getCentreX() - 2.0f, led.getCentreY() - 2.0f,
                                              colours::ledOn, led.getCentreX(), led.getBottom(), true));
    gr.fillEllipse (led);
    gr.setColour (juce::Colours::black.withAlpha (0.7f));
    gr.drawEllipse (led, 1.0f);

    gr.setColour (colours::label);
    gr.setFont (juce::FontOptions (34.0f, juce::Font::bold));
    gr.drawText ("MB-11", title, juce::Justification::left, false);

    gr.setColour (colours::labelDim);
    gr.setFont (juce::FontOptions (12.0f));
    gr.drawText ("ANALOG BASS DRUM", area.removeFromTop (16.0f).translated (2.0f, 0.0f), juce::Justification::left, false);

    area.removeFromTop (10.0f);
    gr.setColour (colours::red);
    gr.fillRect (area.removeFromTop (2.0f));
    area.removeFromTop (12.0f);

    for (const auto& row : rows)
    {
        auto line = area.removeFromTop (rowHeight);
        gr.setColour (colours::labelDim);
        gr.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        gr.drawText (row.label.toUpperCase(), line.removeFromLeft (96.0f), juce::Justification::left, false);
        gr.setColour (colours::label);
        gr.setFont (juce::FontOptions (15.0f));
        gr.drawText (row.value, line, juce::Justification::left, false);
    }

    gr.setColour (colours::labelDim);
    gr.setFont (juce::FontOptions (10.5f));
    gr.drawText (juce::String::fromUTF8 ("Klick schlie\xc3\x9f" "t"), area.removeFromBottom (14.0f), juce::Justification::right, false);
}

void InfoPopup::mouseUp (const juce::MouseEvent&)
{
    if (onClose != nullptr)
        onClose();
}
}
