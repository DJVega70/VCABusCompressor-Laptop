#include "PluginEditor.h"

namespace
{
    const juce::Colour cream { 0xfff0e6cc }, ink { 0xff231d12 }, redZone { 0xffc2332b };
    const juce::Colour brassLight { 0xffe8cb74 }, brassDark { 0xff8a6a22 };
    const juce::Colour navyTop { 0xff243a60 }, navyBottom { 0xff101a30 };
    constexpr float kPi = juce::MathConstants<float>::pi;
    constexpr float kTwoPi = juce::MathConstants<float>::twoPi;

    void drawScrew(juce::Graphics& g, float x, float y, float turn)
    {
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillEllipse(x - 5.5f, y - 4.0f, 11.0f, 11.0f);
        g.setGradientFill(juce::ColourGradient(brassLight, x - 4.0f, y - 5.0f, brassDark, x + 4.0f, y + 5.0f, false));
        g.fillEllipse(x - 5.5f, y - 5.5f, 11.0f, 11.0f);
        juce::Path slot;
        slot.addRectangle(-4.0f, -0.9f, 8.0f, 1.8f);
        g.setColour(juce::Colour(0xff2b2110));
        g.fillPath(slot, juce::AffineTransform::rotation(turn).translated(x, y));
    }
}

//==============================================================================
void HardwareLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float,
                                           float startAngle, float endAngle, juce::Slider& s)
{
    const auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(1.0f);
    const float d = juce::jmin(b.getWidth(), b.getHeight());
    const auto c = b.getCentre();
    const float angle = startAngle + (float) s.valueToProportionOfLength(s.getValue()) * (endAngle - startAngle);

    // shadow + brass ring
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillEllipse(c.x - d * 0.5f + 1.0f, c.y - d * 0.5f + 4.0f, d, d);
    g.setGradientFill(juce::ColourGradient(brassLight, c.x - d * 0.35f, c.y - d * 0.5f,
                                           brassDark, c.x + d * 0.35f, c.y + d * 0.5f, false));
    g.fillEllipse(c.x - d * 0.5f, c.y - d * 0.5f, d, d);

    // dark skirt
    const float sd = d - 5.0f;
    g.setColour(juce::Colour(0xff0d0e11));
    g.fillEllipse(c.x - sd * 0.5f, c.y - sd * 0.5f, sd, sd);

    // graphite cap
    const float cd = sd - 6.0f;
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff55575f), c.x - cd * 0.3f, c.y - cd * 0.5f,
                                           juce::Colour(0xff16171a), c.x + cd * 0.3f, c.y + cd * 0.5f, false));
    g.fillEllipse(c.x - cd * 0.5f, c.y - cd * 0.5f, cd, cd);

    // knurled edge (turns with the knob)
    g.setColour(juce::Colours::white.withAlpha(0.20f));
    for (int i = 0; i < 30; ++i)
    {
        const float a = angle + (float) i * kTwoPi / 30.0f;
        const float sx = std::sin(a), cy = -std::cos(a);
        g.drawLine(c.x + sx * (cd * 0.5f - 4.5f), c.y + cy * (cd * 0.5f - 4.5f),
                   c.x + sx * (cd * 0.5f - 0.6f), c.y + cy * (cd * 0.5f - 0.6f), 1.0f);
    }

    // flat top face
    const float fd = cd - 12.0f;
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff34363c), c.x, c.y - fd * 0.5f,
                                           juce::Colour(0xff121316), c.x, c.y + fd * 0.5f, false));
    g.fillEllipse(c.x - fd * 0.5f, c.y - fd * 0.5f, fd, fd);
    g.setColour(juce::Colours::white.withAlpha(0.10f));
    g.drawEllipse(c.x - fd * 0.5f, c.y - fd * 0.5f, fd, fd, 1.0f);

    // pointer
    juce::Path pointer;
    pointer.addRoundedRectangle(-1.7f, -fd * 0.5f + 0.5f, 3.4f, fd * 0.46f, 1.3f);
    g.setColour(cream);
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(c.x, c.y));
}

//==============================================================================
Knob::Knob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& title,
           juce::StringArray tickLabels, std::function<juce::String(double)> formatter, HardwareLookAndFeel& laf)
    : ticks(std::move(tickLabels)), fmt(std::move(formatter))
{
    slider.setLookAndFeel(&laf);
    slider.setRotaryParameters(kPi * 1.25f, kPi * 2.75f, true);
    slider.setMouseDragSensitivity(180);
    addAndMakeVisible(slider);

    name.setText(title.toUpperCase(), juce::dontSendNotification);
    name.setJustificationType(juce::Justification::centred);
    name.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    name.setColour(juce::Label::textColourId, cream);
    name.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(name);

    value.setJustificationType(juce::Justification::centred);
    value.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 10.5f, juce::Font::plain));
    value.setColour(juce::Label::textColourId, brassLight.withAlpha(0.8f));
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

        g.setColour(cream.withAlpha(0.9f));
        g.drawLine(c.x + sx * (r0 + 2.0f), c.y + cy * (r0 + 2.0f),
                   c.x + sx * (r0 + 6.5f), c.y + cy * (r0 + 6.5f), 1.4f);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(ticks[i], juce::Rectangle<float>(30.0f, 12.0f).withCentre({ c.x + sx * (r0 + 15.0f), c.y + cy * (r0 + 15.0f) }),
                   juce::Justification::centred);
    }
}

void Knob::resized()
{
    slider.setBounds(juce::Rectangle<int>(58, 58).withCentre({ getWidth() / 2, 54 }));
    name.setBounds(0, 96, getWidth(), 16);
    value.setBounds(0, 111, getWidth(), 14);
}

//==============================================================================
void LedButton::paintButton(juce::Graphics& g, bool over, bool down)
{
    auto b = getLocalBounds().toFloat().reduced(1.0f);
    const bool on = getToggleState();

    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(b.translated(0.0f, 3.0f), 5.0f);

    auto face = down ? b.translated(0.0f, 1.5f) : b;
    g.setColour(juce::Colour(0xff0b0b0d));
    g.fillRoundedRectangle(face, 5.0f);

    auto cap = face.reduced(3.0f);
    auto top = juce::Colour(0xff72757d), bottom = juce::Colour(0xff3a3c42);
    if (over) { top = top.brighter(0.1f); bottom = bottom.brighter(0.1f); }
    g.setGradientFill(juce::ColourGradient(top, 0, cap.getY(), bottom, 0, cap.getBottom(), false));
    g.fillRoundedRectangle(cap, 3.0f);
    g.setColour(juce::Colours::white.withAlpha(0.18f));
    g.drawLine(cap.getX() + 3.0f, cap.getY() + 0.7f, cap.getRight() - 3.0f, cap.getY() + 0.7f, 1.0f);

    // lamp window
    auto win = juce::Rectangle<float>(cap.getWidth() - 24.0f, 9.0f).withCentre({ cap.getCentreX(), cap.getY() + 11.0f });
    g.setColour(juce::Colour(0xff0f0f11));
    g.fillRoundedRectangle(win.expanded(1.5f), 3.0f);
    if (on)
    {
        g.setColour(lamp.withAlpha(0.30f));
        g.fillRoundedRectangle(win.expanded(6.0f, 5.0f), 7.0f);
    }
    g.setColour(on ? lamp : lamp.withAlpha(0.16f));
    g.fillRoundedRectangle(win, 2.0f);
    if (on)
    {
        g.setColour(juce::Colours::white.withAlpha(0.35f));
        g.fillRoundedRectangle(win.withHeight(win.getHeight() * 0.4f).reduced(2.0f, 0.5f), 1.5f);
    }

    g.setColour(cream);
    g.setFont(juce::FontOptions(13.5f, juce::Font::bold));
    g.drawText(getButtonText(), cap.withTrimmedTop(19.0f), juce::Justification::centred);
}

//==============================================================================
void NeedleMeter::timerCallback()
{
    const float target = proc.grMeterDb.load();
    shown += (target - shown) * (target > shown ? 0.40f : 0.08f);   // quick rise, lazy fall
    if (shown < 0.01f) shown = 0.0f;
    repaint();
}

void NeedleMeter::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // brass bezel
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillRoundedRectangle(b.translated(0.0f, 3.0f), 10.0f);
    g.setGradientFill(juce::ColourGradient(brassLight, 0, b.getY(), brassDark, 0, b.getBottom(), false));
    g.fillRoundedRectangle(b, 10.0f);
    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.drawRoundedRectangle(b.reduced(1.0f), 10.0f, 1.0f);

    const auto inner = b.reduced(9.0f);
    g.setColour(juce::Colour(0xff09090a));
    g.fillRoundedRectangle(inner, 6.0f);

    const auto face = inner.reduced(4.0f);
    juce::Path clip;
    clip.addRoundedRectangle(face, 4.0f);

    g.saveState();
    g.reduceClipRegion(clip);

    // cream dial face, lit from the middle
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xfff8efd6), face.getCentreX(), face.getCentreY() - 10.0f,
                                           juce::Colour(0xffd2bd84), face.getX(), face.getY(), true));
    g.fillRect(face);

    const float cx = face.getCentreX();
    const float pivotY = face.getBottom() + 34.0f;
    const float Rs = 168.0f;
    auto angleFor = [](float v) { return juce::degreesToRadians(-40.0f + juce::jlimit(0.0f, 20.0f, v) / 20.0f * 80.0f); };
    auto pt = [&](float a, float r) { return juce::Point<float>(cx + r * std::sin(a), pivotY - r * std::cos(a)); };

    // scale arc: black up to 14 dB, red zone beyond
    {
        juce::Path arc, red;
        arc.addCentredArc(cx, pivotY, Rs + 4.0f, Rs + 4.0f, 0.0f, angleFor(0.0f), angleFor(14.0f), true);
        red.addCentredArc(cx, pivotY, Rs + 4.0f, Rs + 4.0f, 0.0f, angleFor(14.0f), angleFor(20.0f), true);
        g.setColour(ink);
        g.strokePath(arc, juce::PathStrokeType(1.6f));
        g.setColour(redZone);
        g.strokePath(red, juce::PathStrokeType(5.0f));
    }

    // ticks and numbers
    static const int majors[] = { 0, 2, 4, 6, 8, 10, 14, 20 };
    for (int v = 0; v <= 20; ++v)
    {
        bool major = false;
        for (int m : majors) if (m == v) major = true;
        const float a = angleFor((float) v);
        g.setColour(v >= 14 ? redZone : ink);
        g.drawLine(juce::Line<float>(pt(a, Rs + 2.0f), pt(a, Rs - (major ? 11.0f : 5.5f))), major ? 2.0f : 1.0f);
        if (major)
        {
            g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
            g.drawText(juce::String(v), juce::Rectangle<float>(26.0f, 14.0f).withCentre(pt(a, Rs - 25.0f)), juce::Justification::centred);
        }
    }

    g.setColour(ink.withAlpha(0.85f));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("GAIN REDUCTION", juce::Rectangle<float>(cx - 90.0f, pivotY - 104.0f, 180.0f, 14.0f), juce::Justification::centred);
    g.setFont(juce::FontOptions(11.0f, juce::Font::plain));
    g.drawText("dB", juce::Rectangle<float>(cx - 30.0f, pivotY - 89.0f, 60.0f, 14.0f), juce::Justification::centred);

    // needle (with a soft shadow on the dial)
    const float a = angleFor(shown);
    const auto tip = pt(a, Rs + 9.0f);
    g.setColour(juce::Colours::black.withAlpha(0.25f));
    g.drawLine(juce::Line<float>(cx + 3.0f, pivotY + 4.0f, tip.x + 3.0f, tip.y + 4.0f), 2.4f);
    g.setColour(juce::Colour(0xff17120a));
    g.drawLine(juce::Line<float>(cx, pivotY, tip.x, tip.y), 2.0f);

    // movement housing rising from the bottom edge
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff45464c), cx, pivotY - 58.0f, juce::Colour(0xff0c0c0e), cx, pivotY, false));
    g.fillEllipse(cx - 58.0f, pivotY - 58.0f, 116.0f, 116.0f);
    g.setColour(brassDark.withAlpha(0.9f));
    g.drawEllipse(cx - 58.0f, pivotY - 58.0f, 116.0f, 116.0f, 1.5f);
    drawScrew(g, cx, face.getBottom() - 12.0f, 0.5f);

    // corner shading and glass reflection
    g.setGradientFill(juce::ColourGradient(juce::Colours::transparentBlack, face.getCentreX(), face.getCentreY(),
                                           juce::Colours::black.withAlpha(0.30f), face.getX(), face.getY(), true));
    g.fillRect(face);
    juce::Path glare;
    glare.addTriangle(face.getX(), face.getY(), face.getX() + face.getWidth() * 0.62f, face.getY(),
                      face.getX(), face.getY() + face.getHeight() * 0.7f);
    g.setColour(juce::Colours::white.withAlpha(0.10f));
    g.fillPath(glare);
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.22f), 0, face.getY(),
                                           juce::Colours::transparentWhite, 0, face.getCentreY(), false));
    g.fillRect(face.withHeight(face.getHeight() * 0.5f));

    g.restoreState();
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.drawRoundedRectangle(face, 4.0f, 1.6f);
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

    auto add = [this](std::unique_ptr<Knob> k, int col, int row)
    {
        addAndMakeVisible(*k);
        k->setBounds(cell(col, row).expanded(12, 0));   // extra width so outer tick labels are not clipped
        knobs.push_back(std::move(k));
    };

    add(std::make_unique<Knob>(a, "threshold", "Threshold", juce::StringArray { "-30", "-20", "-10", "0" }, db, laf), 0, 0);
    add(std::make_unique<Knob>(a, "ratio", "Ratio", VcaBusCompAudioProcessor::ratioLabels(),
                               stepFmt(VcaBusCompAudioProcessor::ratioLabels(), " : 1"), laf), 1, 0);
    add(std::make_unique<Knob>(a, "attack", "Attack", VcaBusCompAudioProcessor::attackLabels(),
                               stepFmt(VcaBusCompAudioProcessor::attackLabels(), " ms"), laf), 2, 0);
    add(std::make_unique<Knob>(a, "release", "Release", VcaBusCompAudioProcessor::releaseLabels(),
                               [](double v) {
                                   static const char* t[] = { "0.1 s", "0.2 s", "0.4 s", "0.8 s", "1.2 s", "AUTO" };
                                   return juce::String(t[juce::jlimit(0, 5, juce::roundToInt(v))]);
                               }, laf), 3, 0);
    add(std::make_unique<Knob>(a, "schpf", "SC Filter", juce::StringArray { "OFF", "60", "90", "150" },
                               [](double v) {
                                   static const char* t[] = { "OFF", "60 Hz", "90 Hz", "150 Hz" };
                                   return juce::String(t[juce::jlimit(0, 3, juce::roundToInt(v))]);
                               }, laf), 0, 1);
    add(std::make_unique<Knob>(a, "makeup", "Make-up", juce::StringArray { "-5", "0", "5", "10", "15" }, db, laf), 1, 1);
    add(std::make_unique<Knob>(a, "trim", "Out Trim", juce::StringArray { "-18", "-9", "0", "+9", "+18" }, db, laf), 2, 1);
    add(std::make_unique<Knob>(a, "mix", "Mix", juce::StringArray { "0", "50", "100" }, pct, laf), 3, 1);

    addAndMakeVisible(meter);
    meter.setBounds(24, 24, 424, 206);

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

    setSize(472, 656);
}

VcaBusCompAudioProcessorEditor::~VcaBusCompAudioProcessorEditor() {}

void VcaBusCompAudioProcessorEditor::paint(juce::Graphics& g)
{
    // satin navy panel
    g.setGradientFill(juce::ColourGradient(navyTop, 0, 0, navyBottom, 0, (float) getHeight(), false));
    g.fillAll();
    juce::Random rng(11);
    g.setColour(juce::Colours::white.withAlpha(0.022f));
    for (int y = 0; y < getHeight(); y += 2)
        if (rng.nextInt(3) == 0) g.drawHorizontalLine(y, 0.0f, (float) getWidth());
    g.setColour(juce::Colours::white.withAlpha(0.14f));
    g.drawLine(0.5f, 0.5f, (float) getWidth(), 0.5f, 1.0f);
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.drawRect(getLocalBounds(), 2);

    drawScrew(g, 12.0f, 12.0f, 0.3f);
    drawScrew(g, (float) getWidth() - 12.0f, 12.0f, 1.2f);
    drawScrew(g, 12.0f, (float) getHeight() - 12.0f, 2.1f);
    drawScrew(g, (float) getWidth() - 12.0f, (float) getHeight() - 12.0f, 0.8f);

    // engraved gold divider lines
    g.setColour(brassLight.withAlpha(0.55f));
    g.drawHorizontalLine(242, 24.0f, 448.0f);
    g.drawHorizontalLine(512, 24.0f, 448.0f);

    // button captions
    g.setColour(cream.withAlpha(0.85f));
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText("COMPRESSOR IN", 52, 572, 104, 14, juce::Justification::centred);
    g.drawText("SOFT KNEE", 184, 572, 104, 14, juce::Justification::centred);
    g.drawText("HARD KNEE", 316, 572, 104, 14, juce::Justification::centred);

    // brass nameplate
    auto plate = juce::Rectangle<float>(96.0f, 598.0f, 280.0f, 34.0f);
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillRoundedRectangle(plate.translated(0.0f, 2.5f), 4.0f);
    g.setGradientFill(juce::ColourGradient(brassLight, 0, plate.getY(), brassDark, 0, plate.getBottom(), false));
    g.fillRoundedRectangle(plate, 4.0f);
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.drawRoundedRectangle(plate.reduced(1.0f), 4.0f, 1.0f);
    g.setColour(juce::Colour(0xff2a200e));
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText("VCA  BUS  COMPRESSOR", plate, juce::Justification::centred);
}

void VcaBusCompAudioProcessorEditor::resized()
{
    inButton.setBounds(52, 520, 104, 46);
    gButton.setBounds(184, 520, 104, 46);
    eButton.setBounds(316, 520, 104, 46);
}
