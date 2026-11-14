#include "PluginEditor.h"

namespace
{
    const juce::Colour bgTop { 0xff26262b }, bgBottom { 0xff121215 }, edge { 0xff3a3a41 },
                       ink { 0xffe4e4e7 }, dim { 0xff9a9aa3 }, track { 0xff2f2f35 };
}

//==============================================================================
void KnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float,
                                       float startAngle, float endAngle, juce::Slider& s)
{
    const auto accent = s.findColour(juce::Slider::rotarySliderFillColourId);
    const bool stepped = (bool) s.getProperties()["stepped"];
    const auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(2.0f);
    const float dimn = juce::jmin(b.getWidth(), b.getHeight());
    const auto c = b.getCentre();

    const double prop = s.valueToProportionOfLength(s.getValue());
    const float angle = startAngle + (float) prop * (endAngle - startAngle);

    // skirt
    g.setColour(juce::Colour(0xff0c0c0e));
    g.fillEllipse(c.x - dimn * 0.5f, c.y - dimn * 0.5f, dimn, dimn);
    g.setColour(edge);
    g.drawEllipse(c.x - dimn * 0.5f, c.y - dimn * 0.5f, dimn, dimn, 1.0f);

    if (!stepped)
    {
        const float r = dimn * 0.5f - 4.0f;
        juce::Path t, f;
        t.addCentredArc(c.x, c.y, r, r, 0.0f, startAngle, endAngle, true);
        g.setColour(track);
        g.strokePath(t, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (prop > 0.001)
        {
            f.addCentredArc(c.x, c.y, r, r, 0.0f, startAngle, angle, true);
            g.setColour(accent);
            g.strokePath(f, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    // cap
    const float capD = dimn - (stepped ? 8.0f : 18.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff4a4a52), c.x, c.y - capD * 0.5f,
                                           juce::Colour(0xff222226), c.x, c.y + capD * 0.5f, false));
    g.fillEllipse(c.x - capD * 0.5f, c.y - capD * 0.5f, capD, capD);
    g.setColour(juce::Colour(0xff5a5a63).withAlpha(0.6f));
    g.drawEllipse(c.x - capD * 0.5f, c.y - capD * 0.5f, capD, capD, 1.0f);

    // pointer
    juce::Path pointer;
    pointer.addRoundedRectangle(-1.5f, -capD * 0.5f + 2.0f, 3.0f, capD * 0.38f, 1.5f);
    g.setColour(accent);
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(c.x, c.y));
}

//==============================================================================
Knob::Knob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& title,
           juce::StringArray tickLabels, bool stepped, juce::Colour accent,
           std::function<juce::String(double)> formatter, KnobLookAndFeel& laf)
    : ticks(std::move(tickLabels)), fmt(std::move(formatter))
{
    slider.setLookAndFeel(&laf);
    slider.getProperties().set("stepped", stepped);
    slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity(stepped ? 110 : 200);
    addAndMakeVisible(slider);

    name.setText(title.toUpperCase(), juce::dontSendNotification);
    name.setJustificationType(juce::Justification::centred);
    name.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    name.setColour(juce::Label::textColourId, ink);
    name.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(name);

    value.setJustificationType(juce::Justification::centred);
    value.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
    value.setColour(juce::Label::textColourId, dim);
    value.setColour(juce::Label::backgroundColourId, juce::Colour(0xff0c0c0e));
    value.setColour(juce::Label::outlineColourId, track);
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
    const float radius = (float) slider.getWidth() * 0.5f + 14.0f;
    const float a0 = juce::MathConstants<float>::pi * 1.25f, a1 = juce::MathConstants<float>::pi * 2.75f;
    const int n = ticks.size();

    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    for (int i = 0; i < n; ++i)
    {
        const float a = a0 + (n > 1 ? (float) i / (float) (n - 1) : 0.0f) * (a1 - a0);
        const float px = c.x + radius * std::sin(a), py = c.y - radius * std::cos(a);
        g.setColour(dim);
        g.drawText(ticks[i], juce::Rectangle<float>(30.0f, 14.0f).withCentre({ px, py }), juce::Justification::centred);
    }
}

void Knob::resized()
{
    const int dimn = 78;
    slider.setBounds(juce::Rectangle<int>(dimn, dimn).withCentre({ getWidth() / 2, 66 }));
    name.setBounds(0, 124, getWidth(), 16);
    value.setBounds(getLocalBounds().withTop(143).withHeight(18).reduced(20, 0));
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
    g.setColour(juce::Colour(0xff0c0c0e));
    g.fillRoundedRectangle(b, 8.0f);
    g.setColour(track);
    g.drawRoundedRectangle(b.reduced(0.5f), 8.0f, 1.0f);

    static const float steps[] = { 0.5f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 14.0f, 20.0f };
    static const char* labels[] = { "0", "2", "4", "6", "8", "10", "14", "20" };
    const int n = 8;

    g.setColour(dim);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText("GAIN REDUCTION  dB", 14, 6, 200, 14, juce::Justification::left);
    g.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::bold));
    g.setColour(ink);
    g.drawText((shown < 0.05f ? juce::String("0.0") : "-" + juce::String(shown, 1)) + " dB", getWidth() - 100, 5, 86, 16, juce::Justification::right);

    const float left = 24.0f, right = (float) getWidth() - 24.0f;
    const float slot = (right - left) / (float) n;
    for (int i = 0; i < n; ++i)
    {
        const bool on = shown >= steps[i];
        const auto col = i < 5 ? juce::Colour(0xfff5c518) : (i < 7 ? juce::Colour(0xfffb923c) : juce::Colour(0xffef4444));
        const float cx = left + slot * ((float) i + 0.5f), cy = b.getHeight() * 0.58f;
        if (on)
        {
            g.setColour(col.withAlpha(0.25f));
            g.fillEllipse(cx - 13.0f, cy - 13.0f, 26.0f, 26.0f);
        }
        g.setColour(on ? col : col.withAlpha(0.14f));
        g.fillEllipse(cx - 7.0f, cy - 7.0f, 14.0f, 14.0f);
        g.setColour(dim);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText(labels[i], juce::Rectangle<float>(24.0f, 12.0f).withCentre({ cx, cy + 20.0f }), juce::Justification::centred);
    }
}

//==============================================================================
VcaBusCompAudioProcessorEditor::VcaBusCompAudioProcessorEditor(VcaBusCompAudioProcessor& p)
    : AudioProcessorEditor(&p), proc(p), meter(p)
{
    auto& a = proc.apvts;
    const auto stepFmt = [](const juce::StringArray& labels, const juce::String& suffix) {
        return [labels, suffix](double v) { return labels[juce::jlimit(0, labels.size() - 1, juce::roundToInt(v))] + suffix; };
    };
    const auto db = [](double v) { return (v > 0.04 ? "+" : "") + juce::String(v, 1) + " dB"; };
    const auto pct = [](double v) { return juce::String(juce::roundToInt(v)) + " %"; };

    const juce::Colour amber { 0xfff5c518 }, red { 0xffef4444 }, blue { 0xff60a5fa },
                       green { 0xff34d399 }, violet { 0xffc084fc }, orange { 0xfffb923c };

    auto add = [this](std::unique_ptr<Knob> k) { addAndMakeVisible(*k); knobs.push_back(std::move(k)); };

    add(std::make_unique<Knob>(a, "threshold", "Threshold", juce::StringArray { "-30", "-20", "-10", "0" }, false, red, db, laf));
    add(std::make_unique<Knob>(a, "ratio",   "Ratio",   VcaBusCompAudioProcessor::ratioLabels(),   true, amber,
                               stepFmt(VcaBusCompAudioProcessor::ratioLabels(), " : 1"), laf));
    add(std::make_unique<Knob>(a, "attack",  "Attack",  VcaBusCompAudioProcessor::attackLabels(),  true, blue,
                               stepFmt(VcaBusCompAudioProcessor::attackLabels(), " ms"), laf));
    add(std::make_unique<Knob>(a, "release", "Release", VcaBusCompAudioProcessor::releaseLabels(), true, green,
                               [](double v) {
                                   static const char* t[] = { "0.1 s", "0.2 s", "0.4 s", "0.8 s", "1.2 s", "AUTO" };
                                   return juce::String(t[juce::jlimit(0, 5, juce::roundToInt(v))]);
                               }, laf));
    add(std::make_unique<Knob>(a, "makeup",  "Make-up", juce::StringArray { "-5", "0", "5", "10", "15" }, false, orange, db, laf));
    add(std::make_unique<Knob>(a, "mix",     "Mix",     juce::StringArray { "0", "50", "100" }, false, violet, pct, laf));

    addAndMakeVisible(meter);

    auto setupCombo = [this](juce::ComboBox& box, juce::Label& label, const juce::String& text, const char* id)
    {
        if (auto* cp = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(id)))
            box.addItemList(cp->choices, 1);
        box.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff0c0c0e));
        box.setColour(juce::ComboBox::textColourId, ink);
        box.setColour(juce::ComboBox::outlineColourId, track);
        box.setColour(juce::ComboBox::arrowColourId, dim);
        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        label.setColour(juce::Label::textColourId, dim);
        addAndMakeVisible(box);
        addAndMakeVisible(label);
    };
    setupCombo(modeBox, modeLabel, "VOICING", "mode");
    setupCombo(hpfBox, hpfLabel, "SIDECHAIN HIGH-PASS", "schpf");
    modeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(proc.apvts, "mode", modeBox);
    hpfAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(proc.apvts, "schpf", hpfBox);

    setSize(900, 400);
}

VcaBusCompAudioProcessorEditor::~VcaBusCompAudioProcessorEditor() {}

void VcaBusCompAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.setGradientFill(juce::ColourGradient(bgTop, 0, 0, bgBottom, 0, (float) getHeight(), false));
    g.fillAll();

    auto card = getLocalBounds().toFloat().reduced(10.0f);
    g.setColour(edge);
    g.drawRoundedRectangle(card, 14.0f, 1.0f);

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("VCA BUS COMPRESSOR", 34, 26, 500, 30, juce::Justification::left);
    g.setColour(dim);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("Stereo-linked feed-forward VCA compressor  -  modeled on console bus compressor behavior",
               34, 56, 700, 18, juce::Justification::left);

    g.setColour(juce::Colour(0xfff5c518));
    g.fillRect(34, 84, 56, 3);
}

void VcaBusCompAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced(34);
    r.removeFromTop(64);

    auto top = r.removeFromTop(78);
    auto right = top.removeFromRight(250);
    meter.setBounds(top.withTrimmedRight(16));

    modeLabel.setBounds(right.removeFromTop(14));
    modeBox.setBounds(right.removeFromTop(26));
    right.removeFromTop(6);
    hpfLabel.setBounds(right.removeFromTop(14));
    hpfBox.setBounds(right.removeFromTop(26).withWidth(110));

    r.removeFromTop(22);
    const int n = (int) knobs.size();
    const int slot = r.getWidth() / n;
    for (int i = 0; i < n; ++i)
        knobs[(size_t) i]->setBounds(r.getX() + i * slot, r.getY(), slot, 170);
}
