#include "PluginEditor.h"

namespace
{
    const juce::Colour silk { 0xffeceff1 };
    const juce::Colour panelTop { 0xff7d838a }, panelBottom { 0xff656b72 };
    const juce::Colour wellDark { 0xff0b0c0e };
    constexpr float kPi = juce::MathConstants<float>::pi;
}

//==============================================================================
void StripLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float,
                                        float startAngle, float endAngle, juce::Slider& s)
{
    const auto cap = s.findColour(juce::Slider::rotarySliderFillColourId);
    const auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(1.0f);
    const float d = juce::jmin(b.getWidth(), b.getHeight());
    const auto c = b.getCentre();
    const float angle = startAngle + (float) s.valueToProportionOfLength(s.getValue()) * (endAngle - startAngle);

    // skirt (dark, with soft shadow underneath)
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillEllipse(c.x - d * 0.5f + 1.0f, c.y - d * 0.5f + 3.0f, d, d);
    g.setColour(juce::Colour(0xff141416));
    g.fillEllipse(c.x - d * 0.5f, c.y - d * 0.5f, d, d);

    // cap
    const float cd = d - 8.0f;
    g.setGradientFill(juce::ColourGradient(cap.brighter(0.35f), c.x - cd * 0.25f, c.y - cd * 0.5f,
                                           cap.darker(0.55f), c.x + cd * 0.25f, c.y + cd * 0.5f, false));
    g.fillEllipse(c.x - cd * 0.5f, c.y - cd * 0.5f, cd, cd);
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.drawEllipse(c.x - cd * 0.5f, c.y - cd * 0.5f, cd, cd, 1.2f);

    // top face
    const float fd = cd - 10.0f;
    g.setGradientFill(juce::ColourGradient(cap.brighter(0.15f), c.x, c.y - fd * 0.5f,
                                           cap.darker(0.25f), c.x, c.y + fd * 0.5f, false));
    g.fillEllipse(c.x - fd * 0.5f, c.y - fd * 0.5f, fd, fd);
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawEllipse(c.x - fd * 0.5f, c.y - fd * 0.5f, fd, fd, 1.0f);

    // pointer line: white on dark caps, black on light caps
    const bool lightCap = cap.getBrightness() > 0.7f;
    juce::Path pointer;
    pointer.addRoundedRectangle(-1.6f, -cd * 0.5f + 1.5f, 3.2f, cd * 0.46f, 1.2f);
    g.setColour(lightCap ? juce::Colour(0xff17171a) : juce::Colours::white);
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(c.x, c.y));
}

//==============================================================================
Knob::Knob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& title,
           juce::StringArray tickLabels, juce::Colour cap, bool large,
           std::function<juce::String(double)> formatter, StripLookAndFeel& laf)
    : isLarge(large), ticks(std::move(tickLabels)), fmt(std::move(formatter))
{
    slider.setLookAndFeel(&laf);
    slider.setColour(juce::Slider::rotarySliderFillColourId, cap);
    slider.setRotaryParameters(kPi * 1.25f, kPi * 2.75f, true);
    slider.setMouseDragSensitivity(180);
    addAndMakeVisible(slider);

    name.setText(title.toUpperCase(), juce::dontSendNotification);
    name.setJustificationType(juce::Justification::centred);
    name.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    name.setColour(juce::Label::textColourId, silk);
    name.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(name);

    value.setJustificationType(juce::Justification::centred);
    value.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 10.5f, juce::Font::plain));
    value.setColour(juce::Label::textColourId, silk.withAlpha(0.62f));
    value.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(value);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);
    if (auto* p = apvts.getParameter(paramId))
        slider.setDoubleClickReturnValue(true, p->convertFrom0to1(p->getDefaultValue()));

    slider.onValueChange = [this] { value.setText(fmt(slider.getValue()), juce::dontSendNotification); };
    slider.onValueChange();
}

void Knob::paint(juce::Graphics& g)
{
    const auto c = slider.getBounds().getCentre().toFloat();
    const float r0 = (float) slider.getWidth() * 0.5f;
    const int n = ticks.size();

    for (int i = 0; i < n; ++i)
    {
        const float t = n > 1 ? (float) i / (float) (n - 1) : 0.5f;
        const float a = kPi * 1.25f + t * (kPi * 1.5f);
        const float sx = std::sin(a), cy = -std::cos(a);

        g.setColour(silk.withAlpha(0.9f));
        g.drawLine(c.x + sx * (r0 + 2.0f), c.y + cy * (r0 + 2.0f),
                   c.x + sx * (r0 + 6.5f), c.y + cy * (r0 + 6.5f), 1.4f);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(ticks[i], juce::Rectangle<float>(30.0f, 12.0f).withCentre({ c.x + sx * (r0 + 15.0f), c.y + cy * (r0 + 15.0f) }),
                   juce::Justification::centred);
    }
}

void Knob::resized()
{
    const int d = isLarge ? 68 : 58;
    slider.setBounds(juce::Rectangle<int>(d, d).withCentre({ getWidth() / 2, 52 }));
    name.setBounds(0, 94, getWidth(), 16);
    value.setBounds(0, 109, getWidth(), 14);
}

//==============================================================================
void LedButton::paintButton(juce::Graphics& g, bool over, bool down)
{
    auto b = getLocalBounds().toFloat().reduced(1.0f);
    const bool on = getToggleState();

    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillRoundedRectangle(b.translated(0.0f, 1.5f), 3.0f);

    auto face = juce::Colour(0xff34373c);
    if (over) face = face.brighter(0.08f);
    if (down) face = face.darker(0.25f);
    g.setGradientFill(juce::ColourGradient(face.brighter(0.12f), 0, b.getY(), face.darker(0.2f), 0, b.getBottom(), false));
    g.fillRoundedRectangle(b, 3.0f);
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.drawRoundedRectangle(b, 3.0f, 1.0f);

    auto ledArea = b.reduced(7.0f, 0.0f).removeFromTop(6.0f).translated(0.0f, 4.0f);
    if (on)
    {
        g.setColour(led.withAlpha(0.28f));
        g.fillRoundedRectangle(ledArea.expanded(3.0f, 3.0f), 4.0f);
    }
    g.setColour(on ? led : led.withAlpha(0.15f));
    g.fillRoundedRectangle(ledArea, 2.0f);

    g.setColour(silk);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText(getButtonText(), b.withTrimmedTop(11.0f), juce::Justification::centred);
}

//==============================================================================
void GrMeter::timerCallback()
{
    const float now = proc.grMeterDb.load();
    shown = now > shown ? now : shown * 0.88f + now * 0.12f;
    repaint();
}

void GrMeter::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(wellDark);
    g.fillRoundedRectangle(b, 4.0f);
    g.setColour(juce::Colours::black);
    g.drawRoundedRectangle(b.reduced(0.5f), 4.0f, 1.5f);

    g.setColour(silk.withAlpha(0.75f));
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText("GAIN", juce::Rectangle<float>(0.0f, 8.0f, (float) getWidth(), 12.0f), juce::Justification::centred);
    g.drawText("REDUCTION", juce::Rectangle<float>(0.0f, 21.0f, (float) getWidth(), 12.0f), juce::Justification::centred);

    static const float steps[] = { 0.5f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 14.0f, 20.0f };
    static const char* labels[] = { "0", "2", "4", "6", "8", "10", "14", "20" };
    const int n = 8;
    const float top = 58.0f, bottom = (float) getHeight() - 64.0f;
    const float pitch = (bottom - top) / (float) (n - 1);
    const float ledX = (float) getWidth() * 0.62f;

    for (int i = 0; i < n; ++i)
    {
        const bool on = shown >= steps[i];
        const auto col = i < 5 ? juce::Colour(0xfff5c518) : (i < 7 ? juce::Colour(0xfffb923c) : juce::Colour(0xffef4444));
        const float cy = top + pitch * (float) i;

        if (on)
        {
            g.setColour(col.withAlpha(0.25f));
            g.fillEllipse(ledX - 14.0f, cy - 14.0f, 28.0f, 28.0f);
        }
        g.setColour(on ? col : col.withAlpha(0.13f));
        g.fillEllipse(ledX - 8.0f, cy - 8.0f, 16.0f, 16.0f);
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawEllipse(ledX - 8.0f, cy - 8.0f, 16.0f, 16.0f, 1.0f);

        g.setColour(silk.withAlpha(0.8f));
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(labels[i], juce::Rectangle<float>(22.0f, 12.0f).withCentre({ ledX - 28.0f, cy }), juce::Justification::centredRight);
    }

    g.setColour(silk.withAlpha(0.85f));
    g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::bold));
    g.drawText((shown < 0.05f ? juce::String("0.0") : "-" + juce::String(shown, 1)),
               juce::Rectangle<float>(0.0f, (float) getHeight() - 44.0f, (float) getWidth(), 16.0f), juce::Justification::centred);
    g.setColour(silk.withAlpha(0.55f));
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText("dB", juce::Rectangle<float>(0.0f, (float) getHeight() - 28.0f, (float) getWidth(), 12.0f), juce::Justification::centred);
}

//==============================================================================
VcaBusCompAudioProcessorEditor::VcaBusCompAudioProcessorEditor(VcaBusCompAudioProcessor& p)
    : AudioProcessorEditor(&p), proc(p), meter(p)
{
    auto& a = proc.apvts;
    const auto stepFmt = [](const juce::StringArray& labels, const juce::String& suffix) {
        return [labels, suffix](double v) { return labels[juce::jlimit(0, labels.size() - 1, juce::roundToInt(v))] + suffix; };
    };
    const auto db  = [](double v) { return (v > 0.04 ? "+" : "") + juce::String(v, 1) + " dB"; };
    const auto pct = [](double v) { return juce::String(juce::roundToInt(v)) + " %"; };

    const juce::Colour red { 0xffd8362c }, orange { 0xffe0861c }, blue { 0xff3d82d8 }, green { 0xff2f9e68 },
                       white { 0xffd9dbde }, black { 0xff2a2c30 }, slate { 0xff56688a };

    auto add = [this](std::unique_ptr<Knob> k, int col, int row)
    {
        addAndMakeVisible(*k);
        k->setBounds(cell(col, row));
        knobs.push_back(std::move(k));
    };

    add(std::make_unique<Knob>(a, "threshold", "Threshold", juce::StringArray { "-30", "-20", "-10", "0" }, red, true, db, laf), 0, 0);
    add(std::make_unique<Knob>(a, "ratio", "Ratio", VcaBusCompAudioProcessor::ratioLabels(), orange, true,
                               stepFmt(VcaBusCompAudioProcessor::ratioLabels(), " : 1"), laf), 1, 0);
    add(std::make_unique<Knob>(a, "attack", "Attack", VcaBusCompAudioProcessor::attackLabels(), blue, false,
                               stepFmt(VcaBusCompAudioProcessor::attackLabels(), " ms"), laf), 0, 1);
    add(std::make_unique<Knob>(a, "release", "Release", VcaBusCompAudioProcessor::releaseLabels(), green, false,
                               [](double v) {
                                   static const char* t[] = { "0.1 s", "0.2 s", "0.4 s", "0.8 s", "1.2 s", "AUTO" };
                                   return juce::String(t[juce::jlimit(0, 5, juce::roundToInt(v))]);
                               }, laf), 1, 1);
    add(std::make_unique<Knob>(a, "makeup", "Make-up", juce::StringArray { "-5", "0", "5", "10", "15" }, white, false, db, laf), 0, 2);
    add(std::make_unique<Knob>(a, "schpf", "SC High-Pass", juce::StringArray { "OFF", "60", "90", "150" }, slate, false,
                               [](double v) {
                                   static const char* t[] = { "OFF", "60 Hz", "90 Hz", "150 Hz" };
                                   return juce::String(t[juce::jlimit(0, 3, juce::roundToInt(v))]);
                               }, laf), 1, 2);
    add(std::make_unique<Knob>(a, "mix", "Mix", juce::StringArray { "0", "50", "100" }, black, false, pct, laf), 0, 3);

    addAndMakeVisible(meter);
    meter.setBounds(268, 168, 72, 504);

    inButton.setClickingTogglesState(true);
    addAndMakeVisible(inButton);
    inAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(a, "in", inButton);

    addAndMakeVisible(gButton);
    addAndMakeVisible(eButton);
    modeAtt = std::make_unique<juce::ParameterAttachment>(*a.getParameter("mode"), [this](float v)
    {
        gButton.setToggleState(v < 0.5f, juce::dontSendNotification);
        eButton.setToggleState(v >= 0.5f, juce::dontSendNotification);
    });
    gButton.onClick = [this] { modeAtt->setValueAsCompleteGesture(0.0f); };
    eButton.onClick = [this] { modeAtt->setValueAsCompleteGesture(1.0f); };
    modeAtt->sendInitialUpdate();

    setSize(360, 704);
}

VcaBusCompAudioProcessorEditor::~VcaBusCompAudioProcessorEditor() {}

void VcaBusCompAudioProcessorEditor::paint(juce::Graphics& g)
{
    // brushed-metal style panel
    g.setGradientFill(juce::ColourGradient(panelTop, 0, 0, panelBottom, 0, (float) getHeight(), false));
    g.fillAll();
    juce::Random rng(7);
    g.setColour(juce::Colours::white.withAlpha(0.025f));
    for (int y = 0; y < getHeight(); y += 2)
        if (rng.nextInt(3) == 0) g.drawHorizontalLine(y, 0.0f, (float) getWidth());
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRect(getLocalBounds(), 2);

    // screws
    for (auto pt : { juce::Point<float>(14, 14), juce::Point<float>(346, 14), juce::Point<float>(14, 690), juce::Point<float>(346, 690) })
    {
        g.setColour(juce::Colour(0xff2c2e33));
        g.fillEllipse(pt.x - 5.0f, pt.y - 5.0f, 10.0f, 10.0f);
        g.setColour(juce::Colour(0xff8b9097));
        g.drawEllipse(pt.x - 5.0f, pt.y - 5.0f, 10.0f, 10.0f, 1.0f);
        g.setColour(juce::Colour(0xff15161a));
        g.drawLine(pt.x - 3.0f, pt.y + 1.5f, pt.x + 3.0f, pt.y - 1.5f, 1.5f);
    }

    // nameplate
    auto plate = juce::Rectangle<float>(20.0f, 26.0f, 320.0f, 58.0f);
    g.setColour(wellDark);
    g.fillRoundedRectangle(plate, 4.0f);
    g.setColour(juce::Colour(0xfff5c518));
    g.fillRect(plate.getX() + 14.0f, plate.getBottom() - 11.0f, 34.0f, 2.5f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
    g.drawText("VCA BUS COMPRESSOR", plate.withTrimmedTop(7.0f).withTrimmedLeft(14.0f).withHeight(26.0f), juce::Justification::left);
    g.setColour(silk.withAlpha(0.55f));
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText("STEREO-LINKED  -  FEED-FORWARD  -  VCA", plate.withTrimmedLeft(54.0f).withTrimmedTop(40.0f).withHeight(14.0f), juce::Justification::left);

    // IN button caption
    g.setColour(silk);
    g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawText("COMPRESSOR IN", 92, 98, 160, 28, juce::Justification::centredLeft);

    // section header: DYNAMICS with rules
    const int hy = 146;
    g.setColour(silk.withAlpha(0.85f));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("DYNAMICS", 0, hy - 8, getWidth(), 16, juce::Justification::centred);
    g.setColour(silk.withAlpha(0.6f));
    g.drawHorizontalLine(hy, 20.0f, 130.0f);
    g.drawHorizontalLine(hy, 230.0f, 340.0f);
    g.drawVerticalLine(20, (float) hy, (float) hy + 8.0f);
    g.drawVerticalLine(339, (float) hy, (float) hy + 8.0f);

    // voicing caption beside the G / E buttons
    const auto vc = cell(1, 3);
    g.setColour(silk);
    g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    g.drawText("VOICING", vc.getX(), vc.getY() + 94, vc.getWidth(), 16, juce::Justification::centred);
    g.setColour(silk.withAlpha(0.62f));
    g.setFont(juce::FontOptions(9.5f, juce::Font::plain));
    g.drawText("G soft  /  E hard knee", vc.getX(), vc.getY() + 109, vc.getWidth(), 14, juce::Justification::centred);
}

void VcaBusCompAudioProcessorEditor::resized()
{
    inButton.setBounds(24, 98, 60, 30);
    const auto vc = cell(1, 3);
    gButton.setBounds(vc.getX() + 6, vc.getY() + 36, 50, 34);
    eButton.setBounds(vc.getX() + 64, vc.getY() + 36, 50, 34);
}
