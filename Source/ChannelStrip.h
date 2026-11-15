#pragma once
// Console channel strip DSP (no JUCE dependency, so it can be unit-tested on its own).
//
// Signal path (stereo, left and right are never mixed together):
//   polarity -> preamp gain (0..72 dB) -> 4x-oversampled saturation (transformer / class-A)
//   -> high-pass / low-pass -> gate -> [compressor] -> 4-band EQ -> [compressor] -> fader -> balance
//
// Latency is a constant 48 samples at every sample rate (the oversampler's linear-phase filters).
// This is a behavioural model "in the spirit of" large-format consoles, not a circuit emulation.

#include "BusCompressor.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace strip
{
constexpr double kPi = 3.14159265358979323846;
constexpr int kLatencySamples = 48;

enum SatMode { Transformer = 0, ClassA = 1 };

struct Params
{
    // preamp
    float preGainDb = 0.0f;      // 0..72
    bool  polarity  = false;     // true = inverted
    int   satMode   = Transformer;
    float satAmount = 0.0f;      // 0..1, 0 = fully linear

    // filters (0 = off)
    float hpfHz = 0.0f;
    float lpfHz = 0.0f;

    // gate / expander
    bool  gateOn = false;
    float gateThrDb = -50.0f, gateRangeDb = -40.0f, gateHoldMs = 20.0f, gateReleaseMs = 150.0f;

    // compressor
    bool  compOn = false;
    bool  compPostEq = false;
    float compThrDb = -18.0f, compRatio = 4.0f, compAttackMs = 10.0f, compReleaseMs = 200.0f, compMakeupDb = 0.0f;

    // EQ
    bool  eqOn = true;
    bool  eqPropQ = false;       // proportional-Q ("smooth") mode
    float lfGainDb = 0.0f,  lfFreq = 100.0f;   bool lfBell = false;
    float lmfGainDb = 0.0f, lmfFreq = 600.0f,  lmfQ = 1.0f;
    float hmfGainDb = 0.0f, hmfFreq = 3000.0f, hmfQ = 1.0f;
    float hfGainDb = 0.0f,  hfFreq = 8000.0f;  bool hfBell = false;

    // output
    float outDb = 0.0f;          // fader, <= -80 = mute
    float pan = 0.0f;            // balance, -1..+1
};

// ---------------------------------------------------------------- biquad helpers
struct Coeffs { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

inline Coeffs normalise(double b0, double b1, double b2, double a0, double a1, double a2)
{
    return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
}

inline Coeffs peakCoeffs(double sr, double f, double gDb, double q)
{
    f = std::min(f, sr * 0.45);
    const double A = std::pow(10.0, gDb / 40.0), w = 2 * kPi * f / sr, c = std::cos(w);
    const double al = std::sin(w) / (2 * q);
    return normalise(1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
}

inline Coeffs lowShelfCoeffs(double sr, double f, double gDb, double q)
{
    f = std::min(f, sr * 0.45);
    const double A = std::pow(10.0, gDb / 40.0), w = 2 * kPi * f / sr, c = std::cos(w);
    const double al = std::sin(w) / (2 * q), t = 2 * std::sqrt(A) * al;
    return normalise(A * ((A + 1) - (A - 1) * c + t), 2 * A * ((A - 1) - (A + 1) * c),
                     A * ((A + 1) - (A - 1) * c - t), (A + 1) + (A - 1) * c + t,
                     -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - t);
}

inline Coeffs highShelfCoeffs(double sr, double f, double gDb, double q)
{
    f = std::min(f, sr * 0.45);
    const double A = std::pow(10.0, gDb / 40.0), w = 2 * kPi * f / sr, c = std::cos(w);
    const double al = std::sin(w) / (2 * q), t = 2 * std::sqrt(A) * al;
    return normalise(A * ((A + 1) + (A - 1) * c + t), -2 * A * ((A - 1) + (A + 1) * c),
                     A * ((A + 1) + (A - 1) * c - t), (A + 1) - (A - 1) * c + t,
                     2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - t);
}

inline Coeffs highpassCoeffs(double sr, double f)
{
    f = std::min(f, sr * 0.45);
    const double w = 2 * kPi * f / sr, c = std::cos(w), al = std::sin(w) / (2 * 0.70710678);
    return normalise((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
}

inline Coeffs lowpassCoeffs(double sr, double f)
{
    f = std::min(f, sr * 0.45);
    const double w = 2 * kPi * f / sr, c = std::cos(w), al = std::sin(w) / (2 * 0.70710678);
    return normalise((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
}

// Magnitude response of a biquad (used by tests and the editor's curve display).
inline double magnitude(const Coeffs& k, double sr, double f)
{
    const double w = 2 * kPi * f / sr;
    const double c1 = std::cos(w), s1 = std::sin(w), c2 = std::cos(2 * w), s2 = std::sin(2 * w);
    const double nr = k.b0 + k.b1 * c1 + k.b2 * c2, ni = -(k.b1 * s1 + k.b2 * s2);
    const double dr = 1 + k.a1 * c1 + k.a2 * c2,    di = -(k.a1 * s1 + k.a2 * s2);
    return std::sqrt((nr * nr + ni * ni) / (dr * dr + di * di));
}

struct Biquad
{
    Coeffs k;
    double z1[2] = { 0, 0 }, z2[2] = { 0, 0 };
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    inline double run(int ch, double x)
    {
        const double y = k.b0 * x + z1[ch];
        z1[ch] = k.b1 * x - k.a1 * y + z2[ch];
        z2[ch] = k.b2 * x - k.a2 * y;
        return y;
    }
};

// ---------------------------------------------------------------- 4x oversampler (linear phase, 193 taps)
class Oversampler4
{
public:
    static constexpr int L = 4, N = 193, Mu = (N + L - 1) / L, R = N + 3;

    Oversampler4() { design(); reset(); }

    void reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            std::fill(std::begin(upBuf[c]), std::end(upBuf[c]), 0.0f);
            std::fill(std::begin(dnBuf[c]), std::end(dnBuf[c]), 0.0f);
            upPos[c] = 0;
            dnPos[c] = 0;
        }
    }

    // One input sample -> four samples at 4x rate.
    inline void upsample(int ch, float x, float* out)
    {
        upPos[ch] = (upPos[ch] + Mu - 1) % Mu;
        upBuf[ch][upPos[ch]] = upBuf[ch][upPos[ch] + Mu] = x;
        const float* h = &upBuf[ch][upPos[ch]];
        for (int k = 0; k < L; ++k)
        {
            float acc = 0;
            for (int m = 0; m < Mu; ++m)
                acc += poly[k][m] * h[m];
            out[k] = acc;
        }
    }

    // Four samples at 4x rate -> one output sample.
    inline float downsample(int ch, const float* in)
    {
        for (int k = 0; k < L; ++k)
        {
            dnPos[ch] = (dnPos[ch] + R - 1) % R;
            dnBuf[ch][dnPos[ch]] = dnBuf[ch][dnPos[ch] + R] = in[k];
        }
        // read 3 samples back so the total delay is a whole 48 input samples
        const float* h = &dnBuf[ch][dnPos[ch] + 3];
        float acc = 0;
        for (int j = 0; j < N; ++j)
            acc += taps[j] * h[j];
        return acc;
    }

private:
    static double bessel0(double x)
    {
        double s = 1, t = 1;
        for (int k = 1; k < 40; ++k) { t *= (x / (2 * k)) * (x / (2 * k)); s += t; }
        return s;
    }

    void design()
    {
        const double fc = 0.125, beta = 9.0;       // cutoff = original Nyquist, ~90 dB stopband
        const int mid = (N - 1) / 2;
        double sum = 0;
        double hd[N];
        for (int i = 0; i < N; ++i)
        {
            const double t = i - mid;
            const double s = t == 0 ? 2 * fc : std::sin(2 * kPi * fc * t) / (kPi * t);
            const double r = (double) t / mid;
            hd[i] = s * bessel0(beta * std::sqrt(std::max(0.0, 1 - r * r))) / bessel0(beta);
            sum += hd[i];
        }
        for (int i = 0; i < N; ++i)
            taps[i] = (float) (hd[i] / sum);
        for (int k = 0; k < L; ++k)
            for (int m = 0; m < Mu; ++m)
            {
                const int idx = L * m + k;
                poly[k][m] = idx < N ? (float) (L * hd[idx] / sum) : 0.0f;
            }
    }

    float taps[N];
    float poly[L][Mu];
    float upBuf[2][2 * Mu], dnBuf[2][2 * R];
    int upPos[2], dnPos[2];
};

// ---------------------------------------------------------------- saturation shapes
struct Shaper
{
    // symmetric soft clip with unity small-signal gain
    static inline float tanhUnity(float x, float a)
    {
        return a < 1.0e-4f ? x : std::tanh(a * x) / a;
    }
    // asymmetric (biased) soft clip, DC-offset removed, unity small-signal gain
    static inline float biased(float x, float a, float bias)
    {
        if (a < 1.0e-4f) return x;
        const float t0 = std::tanh(a * bias);
        const float g0 = 1.0f - t0 * t0;
        return (std::tanh(a * (x + bias)) - t0) / (a * g0);
    }
};

// ---------------------------------------------------------------- the strip
class ChannelStrip
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        os.reset();
        for (auto& b : bands) b.reset();
        hpf.reset(); lpf.reset();
        comp.prepare(sr);
        lowState[0] = lowState[1] = 0;
        lowCoef = (float) (1.0 - std::exp(-2 * kPi * 160.0 / (sr * 4)));
        dcR = (float) (1.0 - 2 * kPi * 5.0 / sr);
        dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0;
        preGain = 1.0; outGainL = outGainR = 1.0;
        gateGain = 1.0; gateEnv = 0.0; gateHoldCount = 0; gateOpen = true;
        gateAtk = std::exp(-1.0 / (0.0003 * sr));
        envDecay = std::exp(-1.0 / (0.005 * sr));
        sm = std::exp(-1.0 / (0.005 * sr));
        started = false;
        eqSettled = false;
    }

    void setParams(const Params& p)
    {
        P = p;
        const double rel = std::max(0.005, (double) p.gateReleaseMs * 0.001);
        gateRel = std::exp(-1.0 / (rel * sr));
        gateHoldSamples = (int) (p.gateHoldMs * 0.001 * sr);
        if (!started)
        {
            started = true;
            preGain = dbToLin(p.preGainDb) * (p.polarity ? -1.0 : 1.0);
            {
                const double f = p.outDb <= -79.9f ? 0.0 : dbToLin(p.outDb), pn = std::max(-1.0f, std::min(1.0f, p.pan));
                outGainL = f * std::cos(std::max(0.0, pn) * kPi * 0.5);
                outGainR = f * std::cos(std::max(0.0, -pn) * kPi * 0.5);
            }
            curHp = p.hpfHz; curLp = p.lpfHz;
            snapEq();
            updateFilters(true);
        }
        vcacomp::Params c;
        c.thresholdDb = p.compThrDb; c.ratio = p.compRatio;
        c.attackMs = p.compAttackMs; c.releaseMs = p.compReleaseMs; c.autoRelease = false;
        c.makeupDb = p.compMakeupDb; c.mix = 1.0f; c.scHpfHz = 80.0f; c.kneeDb = 6.0f;
        comp.setParams(c);
    }

    // In-place stereo processing; output is delayed by kLatencySamples.
    void process(float* l, float* r, int n)
    {
        float inPk = 0, outPk = 0;
        const double polar = P.polarity ? -1.0 : 1.0;
        const double preTarget = dbToLin(P.preGainDb) * polar;
        const bool satActive = P.satAmount > 0.0005f;
        const float a = P.satAmount * 2.5f;

        // 1. preamp + saturation (oversampled) -----------------------------------------
        float* ch[2] = { l, r };
        for (int i = 0; i < n; ++i)
        {
            preGain += (preTarget - preGain) * (1.0 - sm);
            for (int c = 0; c < 2; ++c)
            {
                inPk = std::max(inPk, std::fabs(ch[c][i]));
                const float x = (float) (ch[c][i] * preGain);
                float up[4];
                os.upsample(c, x, up);
                if (satActive)
                {
                    for (int k = 0; k < 4; ++k)
                    {
                        if (P.satMode == Transformer)
                        {
                            // low frequencies are driven harder, like core saturation
                            lowState[c] += lowCoef * (up[k] - lowState[c]);
                            const float low = lowState[c], high = up[k] - low;
                            up[k] = Shaper::tanhUnity(low, a * 3.0f) + Shaper::tanhUnity(high, a);
                        }
                        else
                            up[k] = Shaper::biased(up[k], a, 0.35f);
                    }
                }
                float y = os.downsample(c, up);
                if (satActive && P.satMode == ClassA)
                {
                    const float o = y - dcX[c] + dcR * dcY[c];
                    dcX[c] = y; dcY[c] = o; y = o;
                }
                else { dcX[c] = dcY[c] = 0; }
                ch[c][i] = y;
            }
        }

        // 2. filters --------------------------------------------------------------------
        updateFilters(false);
        if (curHp > 1.0f || curLp > 1.0f)
            for (int i = 0; i < n; ++i)
                for (int c = 0; c < 2; ++c)
                {
                    double x = ch[c][i];
                    if (curHp > 1.0f) x = hpf.run(c, x);
                    if (curLp > 1.0f) x = lpf.run(c, x);
                    ch[c][i] = (float) x;
                }

        // 3. gate -----------------------------------------------------------------------
        gateGrDb = 0.0f;
        if (P.gateOn)
        {
            const double thr = dbToLin(P.gateThrDb), thrClose = dbToLin(P.gateThrDb - 3.0);
            const double range = dbToLin(std::min(0.0f, P.gateRangeDb));
            double worst = 1.0;
            for (int i = 0; i < n; ++i)
            {
                const double pk = std::max(std::fabs((double) l[i]), std::fabs((double) r[i]));
                gateEnv = std::max(pk, gateEnv * envDecay);
                if (gateEnv > thr) { gateOpen = true; gateHoldCount = gateHoldSamples; }
                else if (gateEnv < thrClose)
                {
                    if (gateHoldCount > 0) --gateHoldCount; else gateOpen = false;
                }
                const double target = gateOpen ? 1.0 : range;
                const double coef = target > gateGain ? gateAtk : gateRel;
                gateGain = target + coef * (gateGain - target);
                l[i] = (float) (l[i] * gateGain);
                r[i] = (float) (r[i] * gateGain);
                worst = std::min(worst, gateGain);
            }
            gateGrDb = (float) (-20.0 * std::log10(std::max(worst, 1.0e-6)));
        }
        else { gateGain = 1.0; gateOpen = true; gateEnv = 0.0; }

        // 4. dynamics (pre-EQ) ----------------------------------------------------------
        compGrDb = 0.0f;
        if (P.compOn && !P.compPostEq) { comp.process(l, r, n); compGrDb = comp.getBlockMaxGainReductionDb(); }

        // 5. EQ -------------------------------------------------------------------------
        if (P.eqOn)
            for (int i = 0; i < n; i += kChunk)
            {
                const int m = std::min(kChunk, n - i);
                stepEq();
                for (int j = 0; j < m; ++j)
                    for (int c = 0; c < 2; ++c)
                    {
                        double x = ch[c][i + j];
                        for (int b = 0; b < 4; ++b)
                            if (bandActive[b]) x = bands[b].run(c, x);
                        ch[c][i + j] = (float) x;
                    }
            }

        // 6. dynamics (post-EQ) ---------------------------------------------------------
        if (P.compOn && P.compPostEq) { comp.process(l, r, n); compGrDb = comp.getBlockMaxGainReductionDb(); }

        // 7. fader + balance ------------------------------------------------------------
        const double fader = P.outDb <= -79.9f ? 0.0 : dbToLin(P.outDb);
        const double pan = std::max(-1.0f, std::min(1.0f, P.pan));
        const double tL = fader * std::cos(std::max(0.0, pan) * kPi * 0.5);
        const double tR = fader * std::cos(std::max(0.0, -pan) * kPi * 0.5);
        for (int i = 0; i < n; ++i)
        {
            outGainL += (tL - outGainL) * (1.0 - sm);
            outGainR += (tR - outGainR) * (1.0 - sm);
            l[i] = (float) (l[i] * outGainL);
            r[i] = (float) (r[i] * outGainR);
            outPk = std::max(outPk, std::max(std::fabs(l[i]), std::fabs(r[i])));
        }
        inPeak = inPk; outPeak = outPk;
    }

    // meters (valid after each process call)
    float inPeak = 0, outPeak = 0, compGrDb = 0, gateGrDb = 0;

    // exposed so the editor can draw the EQ curve
    Coeffs bandCoeffs(const Params& p, int band) const { return designBand(p, band, sr); }

    static Coeffs designBand(const Params& p, int band, double sr)
    {
        const bool prop = p.eqPropQ;
        auto propQ = [&](double q, double g) { return prop ? q * (0.5 + 0.75 * std::min(1.0, std::fabs(g) / 15.0)) : q; };
        switch (band)
        {
            case 0: return p.lfBell ? peakCoeffs(sr, p.lfFreq, p.lfGainDb, propQ(0.7, p.lfGainDb))
                                    : lowShelfCoeffs(sr, p.lfFreq, p.lfGainDb, 0.6);
            case 1: return peakCoeffs(sr, p.lmfFreq, p.lmfGainDb, propQ(p.lmfQ, p.lmfGainDb));
            case 2: return peakCoeffs(sr, p.hmfFreq, p.hmfGainDb, propQ(p.hmfQ, p.hmfGainDb));
            default: return p.hfBell ? peakCoeffs(sr, p.hfFreq, p.hfGainDb, propQ(0.7, p.hfGainDb))
                                     : highShelfCoeffs(sr, p.hfFreq, p.hfGainDb, 0.6);
        }
    }

private:
    static constexpr int kChunk = 32;
    static double dbToLin(double db) { return std::pow(10.0, db / 20.0); }

    struct Smoothed { double g = 0, f = 0, q = 0; };

    void bandTargets(double* g, double* f, double* q) const
    {
        g[0] = P.lfGainDb;  f[0] = P.lfFreq;  q[0] = 0;
        g[1] = P.lmfGainDb; f[1] = P.lmfFreq; q[1] = P.lmfQ;
        g[2] = P.hmfGainDb; f[2] = P.hmfFreq; q[2] = P.hmfQ;
        g[3] = P.hfGainDb;  f[3] = P.hfFreq;  q[3] = 0;
    }

    void snapEq()
    {
        double g[4], f[4], q[4];
        bandTargets(g, f, q);
        for (int b = 0; b < 4; ++b) { sg[b] = g[b]; sf[b] = f[b]; sq[b] = q[b]; }
        eqSettled = false;
        redesign();
    }

    // one-pole smoothing of EQ controls every chunk, then redesign
    void stepEq()
    {
        double g[4], f[4], q[4];
        bandTargets(g, f, q);
        const double k = 1.0 - std::exp(-(double) kChunk / (0.02 * sr));
        bool changed = false;
        for (int b = 0; b < 4; ++b)
        {
            const double dg = g[b] - sg[b], df = f[b] - sf[b], dq = q[b] - sq[b];
            if (std::fabs(dg) > 1e-4 || std::fabs(df) > 1e-3 || std::fabs(dq) > 1e-4)
            {
                sg[b] += dg * k; sf[b] += df * k; sq[b] += dq * k;
                changed = true;
            }
            else { sg[b] = g[b]; sf[b] = f[b]; sq[b] = q[b]; }
        }
        if (changed || !eqSettled || P.eqPropQ != lastProp || P.lfBell != lastLfBell || P.hfBell != lastHfBell)
        {
            redesign();
            eqSettled = !changed;
        }
    }

    void redesign()
    {
        Params t = P;
        t.lfGainDb = (float) sg[0]; t.lfFreq = (float) sf[0];
        t.lmfGainDb = (float) sg[1]; t.lmfFreq = (float) sf[1]; t.lmfQ = (float) sq[1];
        t.hmfGainDb = (float) sg[2]; t.hmfFreq = (float) sf[2]; t.hmfQ = (float) sq[2];
        t.hfGainDb = (float) sg[3]; t.hfFreq = (float) sf[3];
        for (int b = 0; b < 4; ++b)
        {
            bands[b].k = designBand(t, b, sr);
            bandActive[b] = std::fabs(sg[b]) > 0.005;
        }
        lastProp = P.eqPropQ; lastLfBell = P.lfBell; lastHfBell = P.hfBell;
    }

    void updateFilters(bool force)
    {
        const double k = 1.0 - sm;
        if (force || std::fabs(P.hpfHz - curHp) > 0.01f) { curHp += (float) ((P.hpfHz - curHp) * (force ? 1.0 : k * 20)); if (curHp > 1.0f) hpf.k = highpassCoeffs(sr, curHp); }
        if (force || std::fabs(P.lpfHz - curLp) > 0.01f) { curLp += (float) ((P.lpfHz - curLp) * (force ? 1.0 : k * 20)); if (curLp > 1.0f) lpf.k = lowpassCoeffs(sr, curLp); }
    }

    double sr = 48000.0;
    Params P;
    Oversampler4 os;
    Biquad bands[4], hpf, lpf;
    bool bandActive[4] = { false, false, false, false };
    double sg[4] = {}, sf[4] = {}, sq[4] = {};
    bool eqSettled = false, lastProp = false, lastLfBell = false, lastHfBell = false;
    float curHp = 0, curLp = 0;
    vcacomp::BusCompressor comp;
    float lowState[2] = {}, lowCoef = 0.0f, dcR = 0.999f, dcX[2] = {}, dcY[2] = {};
    double preGain = 1.0, outGainL = 1.0, outGainR = 1.0, sm = 0.99;
    double gateGain = 1.0, gateEnv = 0.0, gateAtk = 0.9, gateRel = 0.99, envDecay = 0.99;
    int gateHoldCount = 0, gateHoldSamples = 0;
    bool gateOpen = true, started = false;
};
} // namespace strip
