#include "PluginEditor.h"

namespace
{
    const juce::Colour panelTop { 0xff34373c }, panelBottom { 0xff1e2024 }, ink { 0xffe9e4d8 }, dim { 0xff9a968c };
    const juce::Colour red { 0xffc8412f }, green { 0xff4f9a64 }, blue { 0xff3f78b8 }, amber { 0xffd7962b },
                       slate { 0xff6a707a }, brown { 0xff7a5638 }, cream { 0xffcfc7b0 };
    constexpr float kPi = juce::MathConstants<float>::pi;

    juce::String hz(double v) { return v >= 1000.0 ? juce::String(v / 1000.0, v >= 10000.0 ? 1 : 2) + "k" : juce::String(juce::roundToInt(v)); }
    juce::String dB(double v) { return (v > 0.05 ? "+" : "") + juce::String(v, 1); }
    juce::String ms(double v) { return v >= 1000.0 ? juce::String(v / 1000.0, 2) + " s" : v >= 10.0 ? juce::String(juce::roundToInt(v)) + " ms" : juce::String(v, 1) + " ms"; }

    void screw(juce::Graphics& g, float x, float y)
    {
        g.setColour(juce::Colours::black.withAlpha(0.5f)); g.fillEllipse(x - 5, y - 4, 10, 10);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff8b8e94), x - 4, y - 5, juce::Colour(0xff3c3e43), x + 4, y + 5, false));
        g.fillEllipse(x - 5, y - 5, 10, 10);
        g.setColour(juce::Colour(0xff1b1c1f)); g.drawLine(x - 3, y + 1, x + 3, y - 1, 1.4f);
    }
}

//==============================================================================
void ConsoleLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float,
                                          float startAngle, float endAngle, juce::Slider& s)
{
    const auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h);
    const float d = juce::jmin(b.getWidth(), b.getHeight()) - 6.0f;
    const auto c = b.getCentre();
    const float prop = (float) s.valueToProportionOfLength(s.getValue());
    const float ang = startAngle + prop * (endAngle - startAngle);
    const juce::Colour cap((juce::uint32) (int) s.getProperties()["cap"]);

    // scale ticks
    g.setColour(dim.withAlpha(0.8f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
        const float r0 = d * 0.5f + 2.5f, r1 = r0 + (i % 5 == 0 ? 4.5f : 2.5f);
        g.drawLine(c.x + std::sin(a) * r0, c.y - std::cos(a) * r0, c.x + std::sin(a) * r1, c.y - std::cos(a) * r1, 1.0f);
    }
    // body
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillEllipse(c.x - d * 0.5f + 1, c.y - d * 0.5f + 3, d, d);
    g.setColour(juce::Colour(0xff0c0d0f));
    g.fillEllipse(c.x - d * 0.5f, c.y - d * 0.5f, d, d);
    const float cd = d - 6.0f;
    g.setGradientFill(juce::ColourGradient(cap.brighter(0.25f), c.x - cd * 0.3f, c.y - cd * 0.5f, cap.darker(0.55f), c.x + cd * 0.3f, c.y + cd * 0.5f, false));
    g.fillEllipse(c.x - cd * 0.5f, c.y - cd * 0.5f, cd, cd);
    g.setColour(juce::Colours::white.withAlpha(0.18f));
    g.drawEllipse(c.x - cd * 0.5f + 0.5f, c.y - cd * 0.5f + 0.5f, cd - 1, cd - 1, 1.0f);
    const float td = cd * 0.62f;
    g.setGradientFill(juce::ColourGradient(cap.brighter(0.05f), c.x, c.y - td * 0.5f, cap.darker(0.3f), c.x, c.y + td * 0.5f, false));
    g.fillEllipse(c.x - td * 0.5f, c.y - td * 0.5f, td, td);
    juce::Path p; p.addRoundedRectangle(-1.4f, -cd * 0.5f + 1.0f, 2.8f, cd * 0.42f, 1.2f);
    g.setColour(juce::Colours::white.withAlpha(0.95f));
    g.fillPath(p, juce::AffineTransform::rotation(ang).translated(c.x, c.y));
}

void ConsoleLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float, float, juce::Slider::SliderStyle, juce::Slider&)
{
    const float cy = (float) y + (float) h * 0.5f;
    auto slot = juce::Rectangle<float>((float) x, cy - 4.0f, (float) w, 8.0f);
    g.setColour(juce::Colour(0xff08090a)); g.fillRoundedRectangle(slot, 4.0f);
    g.setColour(juce::Colours::white.withAlpha(0.12f)); g.drawRoundedRectangle(slot, 4.0f, 1.0f);
    auto cap = juce::Rectangle<float>(22.0f, (float) h - 2.0f).withCentre({ pos, cy });
    g.setColour(juce::Colours::black.withAlpha(0.5f)); g.fillRoundedRectangle(cap.translated(0, 3), 4.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe6e2d6), 0, cap.getY(), juce::Colour(0xff9b988d), 0, cap.getBottom(), false));
    g.fillRoundedRectangle(cap, 4.0f);
    g.setColour(juce::Colour(0xff1c1d1f)); g.fillRect(cap.getCentreX() - 1.0f, cap.getY() + 3.0f, 2.0f, cap.getHeight() - 6.0f);
}

//==============================================================================
StripKnob::StripKnob(juce::AudioProcessorValueTreeState& a, const juce::String& id, const juce::String& t,
                     juce::Colour cap, std::function<juce::String(double)> f, ConsoleLookAndFeel& laf)
    : title(t.toUpperCase()), fmt(std::move(f))
{
    slider.setLookAndFeel(&laf);
    slider.getProperties().set("cap", (int) cap.getARGB());
    slider.setRotaryParameters(kPi * 1.2f, kPi * 2.8f, true);
    slider.setMouseDragSensitivity(170);
    addAndMakeVisible(slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(a, id, slider);
    if (auto* p = a.getParameter(id))
        slider.setDoubleClickReturnValue(true, p->convertFrom0to1(p->getDefaultValue()));
    value.setJustificationType(juce::Justification::centred);
    value.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 10.5f, juce::Font::plain));
    value.setColour(juce::Label::textColourId, ink.withAlpha(0.85f));
    value.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(value);
    slider.onValueChange = [this] { value.setText(fmt(slider.getValue()), juce::dontSendNotification); };
    slider.onValueChange();
}
void StripKnob::paint(juce::Graphics& g)
{
    g.setColour(dim); g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText(title, 0, 0, getWidth(), 13, juce::Justification::centred);
}
void StripKnob::resized()
{
    const int d = juce::jmin(getWidth() - 4, getHeight() - 30);
    slider.setBounds(juce::Rectangle<int>(d, d).withCentre({ getWidth() / 2, 14 + d / 2 }));
    value.setBounds(0, getHeight() - 14, getWidth(), 14);
}

//==============================================================================
void LampButton::paintButton(juce::Graphics& g, bool over, bool down)
{
    auto b = getLocalBounds().toFloat().reduced(1.0f);
    const bool on = getToggleState();
    g.setColour(juce::Colours::black.withAlpha(0.5f)); g.fillRoundedRectangle(b.translated(0, 2), 4.0f);
    auto face = down ? b.translated(0, 1) : b;
    g.setColour(juce::Colour(0xff0b0b0d)); g.fillRoundedRectangle(face, 4.0f);
    auto cap = face.reduced(2.5f);
    juce::Colour t(0xff6c7077), bt(0xff383a3f);
    if (over) { t = t.brighter(0.1f); bt = bt.brighter(0.1f); }
    g.setGradientFill(juce::ColourGradient(t, 0, cap.getY(), bt, 0, cap.getBottom(), false));
    g.fillRoundedRectangle(cap, 3.0f);
    auto lamp = juce::Rectangle<float>(cap.getWidth() - 16.0f, 5.0f).withCentre({ cap.getCentreX(), cap.getY() + 7.0f });
    if (on) { g.setColour(lampColour.withAlpha(0.28f)); g.fillRoundedRectangle(lamp.expanded(4, 3), 5.0f); }
    g.setColour(on ? lampColour : lampColour.withAlpha(0.18f)); g.fillRoundedRectangle(lamp, 2.0f);
    g.setColour(ink); g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    g.drawText(getButtonText(), cap.withTrimmedTop(12.0f), juce::Justification::centred);
}

//==============================================================================
void Ladder::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(dim); g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText(text, b.removeFromLeft(34.0f), juce::Justification::centredLeft);
    const int n = 26; const float gap = 2.0f, sw = (b.getWidth() - gap * (n - 1)) / n;
    for (int i = 0; i < n; ++i)
    {
        float frac = (float) (i + 1) / (float) n;
        bool lit; juce::Colour col;
        if (kind == Peak)   // -60 .. +6 dBFS
        {
            const float lvl = -60.0f + frac * 66.0f;
            lit = shown >= lvl - 1.0f;
            col = lvl > 0.0f ? red : lvl > -12.0f ? amber : green;
        }
        else                // 0 .. 20 dB reduction
        {
            lit = shown >= frac * 20.0f - 0.4f && shown > 0.05f;
            col = amber;
        }
        auto seg = juce::Rectangle<float>(b.getX() + i * (sw + gap), b.getY() + 2, sw, b.getHeight() - 4);
        g.setColour(lit ? col : col.withAlpha(0.13f)); g.fillRoundedRectangle(seg, 1.2f);
    }
}

//==============================================================================
class ConsoleStripEditor::Content : public juce::Component, private juce::Timer
{
public:
    Content(ConsoleStripProcessor& pr) : proc(pr), apvts(pr.apvts)
    {
        setLookAndFeel(&laf);
        auto knob = [&](const char* id, const char* t, juce::Colour c, std::function<juce::String(double)> f) {
            knobs.push_back(std::make_unique<StripKnob>(apvts, id, t, c, std::move(f), laf));
            addAndMakeVisible(*knobs.back()); return knobs.back().get(); };
        auto toggle = [&](const char* id, const juce::String& t, juce::Colour lamp) {
            auto* bt = buttons.emplace_back(std::make_unique<LampButton>(t, lamp)).get();
            addAndMakeVisible(*bt);
            attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, id, *bt));
            bt->setClickingTogglesState(true); return bt; };

                auto db1 = [](double v) { return dB(v); };
        // INPUT
        kPre = knob("pre", "Preamp", red, [](double v) { return juce::String(v, 1) + " dB"; });
        kDrive = knob("drive", "Drive", red, [](double v) { return juce::String(juce::roundToInt(v)) + " %"; });
        bPol = toggle("polarity", juce::String::fromUTF8("\xC3\x98 INV"), red);
        satX = std::make_unique<LampButton>("XFMR", amber); satA = std::make_unique<LampButton>("CLASS-A", amber);
        addAndMakeVisible(*satX); addAndMakeVisible(*satA);
        satAtt = std::make_unique<juce::ParameterAttachment>(*apvts.getParameter("satmode"), [this](float v) {
            satX->setToggleState(v < 0.5f, juce::dontSendNotification); satA->setToggleState(v >= 0.5f, juce::dontSendNotification); });
        satX->onClick = [this] { satAtt->setValueAsCompleteGesture(0.0f); };
        satA->onClick = [this] { satAtt->setValueAsCompleteGesture(1.0f); };
        satAtt->sendInitialUpdate();
        // FILTERS
        kHpf = knob("hpf", "High-pass", slate, [](double v) { return hz(v) + " Hz"; });
        kLpf = knob("lpf", "Low-pass", slate, [](double v) { return hz(v) + " Hz"; });
        bHpf = toggle("hpfon", "HPF IN", green); bLpf = toggle("lpfon", "LPF IN", green);
        // GATE
        bGate = toggle("gateon", "GATE", green);
        knob("gthr", "Thresh", green, [](double v) { return juce::String(v, 1) + " dB"; });
        knob("grange", "Range", green, [](double v) { return juce::String(v, 0) + " dB"; });
        knob("ghold", "Hold", green, [](double v) { return ms(v); });
        knob("grel", "Release", green, [](double v) { return ms(v); });
        // COMP
        bComp = toggle("compon", "COMP", amber); bPost = toggle("comppost", "POST EQ", amber);
        knob("cthr", "Thresh", amber, [](double v) { return juce::String(v, 1) + " dB"; });
        knob("cratio", "Ratio", amber, [](double v) { return juce::String(v, 1) + ":1"; });
        knob("catk", "Attack", amber, [](double v) { return ms(v); });
        knob("crel", "Release", amber, [](double v) { return ms(v); });
        knob("cmakeup", "Make-up", amber, [](double v) { return "+" + juce::String(v, 1) + " dB"; });
        // EQ
        bEq = toggle("eqon", "EQ", blue); bProp = toggle("propq", "PROP Q", blue);
        knob("hfg", "HF gain", red, db1);      knob("hff", "HF freq", red, [](double v) { return hz(v) + " Hz"; });
        bHfBell = toggle("hfbell", "BELL", red);
        knob("hmg", "HMF gain", green, db1);   knob("hmf", "HMF freq", green, [](double v) { return hz(v) + " Hz"; });
        knob("hmq", "HMF Q", green, [](double v) { return juce::String(v, 2); });
        knob("lmg", "LMF gain", blue, db1);    knob("lmf", "LMF freq", blue, [](double v) { return hz(v) + " Hz"; });
        knob("lmq", "LMF Q", blue, [](double v) { return juce::String(v, 2); });
        knob("lfg", "LF gain", brown, db1);    knob("lff", "LF freq", brown, [](double v) { return hz(v) + " Hz"; });
        bLfBell = toggle("lfbell", "BELL", brown);
        // OUTPUT
        knob("pan", "Balance", cream, [](double v) { return v == 0 ? juce::String("C") : (v < 0 ? "L" : "R") + juce::String(juce::roundToInt(std::fabs(v))); });
        fader.setSliderStyle(juce::Slider::LinearHorizontal); fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        fader.setLookAndFeel(&laf); fader.setDoubleClickReturnValue(true, 0.0);
        addAndMakeVisible(fader);
        faderAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "fader", fader);
        faderVal.setJustificationType(juce::Justification::centredRight); faderVal.setColour(juce::Label::textColourId, ink);
        faderVal.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 11.5f, juce::Font::bold));
        addAndMakeVisible(faderVal);
        fader.onValueChange = [this] { faderVal.setText(fader.getValue() <= -79.9 ? "-inf dB" : dB(fader.getValue()) + " dB", juce::dontSendNotification); };
        fader.onValueChange();
        for (auto* l : { &lIn, &lOut, &lComp, &lGate }) addAndMakeVisible(*l);
        startTimerHz(30);
    }
    ~Content() override { stopTimer(); setLookAndFeel(nullptr); fader.setLookAndFeel(nullptr); }

    void paint(juce::Graphics& g) override
    {
        g.setGradientFill(juce::ColourGradient(panelTop, 0, 0, panelBottom, 0, (float) getHeight(), false));
        g.fillAll();
        juce::Random rng(5); g.setColour(juce::Colours::white.withAlpha(0.02f));
        for (int y = 0; y < getHeight(); y += 2) if (rng.nextInt(3) == 0) g.drawHorizontalLine(y, 0.0f, (float) getWidth());
        g.setColour(juce::Colours::black.withAlpha(0.6f)); g.drawRect(getLocalBounds(), 2);
        screw(g, 12, 12); screw(g, getWidth() - 12.0f, 12); screw(g, 12, getHeight() - 12.0f); screw(g, getWidth() - 12.0f, getHeight() - 12.0f);

        // nameplate
        g.setColour(ink); g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
        g.drawText("CONSOLE  CHANNEL  STRIP", 0, 14, getWidth(), 22, juce::Justification::centred);
        g.setColour(dim); g.setFont(juce::FontOptions(9.5f, juce::Font::plain));
        g.drawText("PREAMP  /  FILTERS  /  GATE  /  COMPRESSOR  /  4-BAND EQ  /  FADER", 0, 36, getWidth(), 12, juce::Justification::centred);

        for (auto& s : sections)
        {
            g.setColour(s.colour); g.fillRoundedRectangle(20.0f, (float) s.y + 4.0f, 4.0f, 12.0f, 1.5f);
            g.setColour(ink.withAlpha(0.9f)); g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
            g.drawText(s.title, 30, s.y + 2, 200, 16, juce::Justification::centredLeft);
            g.setColour(juce::Colours::white.withAlpha(0.10f)); g.drawHorizontalLine(s.y + 20, 20.0f, getWidth() - 20.0f);
        }
    }

    void resized() override
    {
        sections.clear();
        auto sec = [&](const juce::String& t, int y, juce::Colour c) { sections.push_back({ t, y, c }); };
        const int x0 = 14, cw = 108;
        auto cell = [&](int col, int y, int h, int cols = 4) { const int w = 432 / cols; return juce::Rectangle<int>(x0 + col * w, y, w, h); };
        int ki = 0;
        auto place = [&](int col, int y, int h, int cols = 4) { knobs[(size_t) ki++]->setBounds(cell(col, y, h, cols)); };

        // INPUT
        sec("INPUT / PREAMP", 52, red);
        place(0, 76, 96); place(1, 76, 96);
        bPol->setBounds(x0 + 2 * cw + 10, 84, 88, 34); satX->setBounds(x0 + 3 * cw + 6, 84, 90, 34);
        satA->setBounds(x0 + 3 * cw + 6, 126, 90, 34);
        // FILTERS
        sec("FILTERS", 176, slate);
        place(0, 200, 96); place(1, 200, 96);
        bHpf->setBounds(x0 + 2 * cw + 10, 208, 88, 34); bLpf->setBounds(x0 + 3 * cw + 6, 208, 90, 34);
        // GATE
        sec("GATE / EXPANDER", 300, green); bGate->setBounds(getWidth() - 94, 296, 74, 28);
        for (int i = 0; i < 4; ++i) place(i, 324, 96);
        // COMP
        sec("COMPRESSOR", 424, amber); bComp->setBounds(getWidth() - 94, 420, 74, 28); bPost->setBounds(getWidth() - 176, 420, 76, 28);
        for (int i = 0; i < 5; ++i) place(i, 448, 96, 5);
        // EQ
        sec("EQUALIZER", 548, blue); bEq->setBounds(getWidth() - 94, 544, 74, 28); bProp->setBounds(getWidth() - 176, 544, 76, 28);
        // columns: HF, HMF, LMF, LF; rows: gain, freq, Q/bell
        const int ey = 572;
        // knob order in vector: hfg,hff,hmg,hmf,hmq,lmg,lmf,lmq,lfg,lff
        const int order[4][2] = { { 0, 1 }, { 2, 3 }, { 5, 6 }, { 8, 9 } };
        for (int c = 0; c < 4; ++c)
        {
            knobs[(size_t) (ki + order[c][0] - 0)]->setBounds(cell(c, ey, 90));
            knobs[(size_t) (ki + order[c][1] - 0)]->setBounds(cell(c, ey + 94, 90));
        }
        knobs[(size_t) (ki + 4)]->setBounds(cell(1, ey + 188, 90));
        knobs[(size_t) (ki + 7)]->setBounds(cell(2, ey + 188, 90));
        bHfBell->setBounds(cell(0, ey + 196, 74).withSizeKeepingCentre(76, 34));
        bLfBell->setBounds(cell(3, ey + 196, 74).withSizeKeepingCentre(76, 34));
        ki += 10;
        // OUTPUT
        sec("OUTPUT", 856, cream);
        knobs[(size_t) ki]->setBounds(cell(0, 878, 92));
        fader.setBounds(x0 + 118, 882, 230, 34); faderVal.setBounds(x0 + 352, 888, 80, 20);
        lIn.setBounds(x0 + 118, 920, 314, 14); lOut.setBounds(x0 + 118, 934, 314, 14);
        lComp.setBounds(x0 + 118, 948, 314, 14);
        lGate.setBounds(x0 + 118, 962, 314, 14);
    }

private:
    void timerCallback() override
    {
        auto toDb = [](float v) { return juce::Decibels::gainToDecibels(v, -80.0f); };
        auto fall = [](float& s, float t) { s = t > s ? t : s - 2.2f; };
        fall(sIn, toDb(proc.inPeak.load())); fall(sOut, toDb(proc.outPeak.load()));
        lIn.setValue(sIn); lOut.setValue(sOut);
        const float gr = juce::jmax(proc.compGr.load(), 0.0f);
        sGr = gr > sGr ? gr : sGr * 0.9f; lComp.setValue(sGr);
        const float gg = juce::jmax(proc.gateGr.load(), 0.0f);
        sGg = gg > sGg ? gg : sGg * 0.9f; lGate.setValue(sGg);
    }

    struct Section { juce::String title; int y; juce::Colour colour; };
    ConsoleStripProcessor& proc;
    juce::AudioProcessorValueTreeState& apvts;
    ConsoleLookAndFeel laf;
    std::vector<std::unique_ptr<StripKnob>> knobs;
    std::vector<std::unique_ptr<LampButton>> buttons;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> attachments;
    std::vector<Section> sections;
    StripKnob *kPre, *kDrive, *kHpf, *kLpf;
    LampButton *bPol, *bHpf, *bLpf, *bGate, *bComp, *bPost, *bEq, *bProp, *bHfBell, *bLfBell;
    std::unique_ptr<LampButton> satX, satA;
    std::unique_ptr<juce::ParameterAttachment> satAtt;
    juce::Slider fader; juce::Label faderVal;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> faderAtt;
    Ladder lIn { Ladder::Peak, "IN" }, lOut { Ladder::Peak, "OUT" }, lComp { Ladder::Reduction, "COMP" }, lGate { Ladder::Reduction, "GATE" };
    float sIn = -80.0f, sOut = -80.0f, sGr = 0.0f, sGg = 0.0f;
};

//==============================================================================
ConsoleStripEditor::ConsoleStripEditor(ConsoleStripProcessor& p) : juce::AudioProcessorEditor(&p)
{
    content = std::make_unique<Content>(p);
    addAndMakeVisible(*content);
    constrainer.setFixedAspectRatio((double) kBaseW / kBaseH);
    constrainer.setSizeLimits(kBaseW / 2, kBaseH / 2, kBaseW * 2, kBaseH * 2);
    setConstrainer(&constrainer);
    setResizable(true, true);
    setSize(kBaseW * 85 / 100, kBaseH * 85 / 100);
}
ConsoleStripEditor::~ConsoleStripEditor() { setConstrainer(nullptr); }

void ConsoleStripEditor::resized()
{
    content->setBounds(0, 0, kBaseW, kBaseH);
    content->setTransform(juce::AffineTransform::scale((float) getWidth() / (float) kBaseW));
}
