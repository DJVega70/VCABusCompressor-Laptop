#pragma once
// Stereo-linked, feed-forward VCA-style bus compressor DSP (no JUCE dependency).
//
// Modeled on the published behavior of the classic 4000-series console bus compressor:
//   * stereo-linked detector (both channels share one gain reduction value)
//   * fixed ratios 2:1 / 4:1 / 10:1, stepped attack and release, "Auto" release
//   * soft knee, sidechain high-pass filter
//   * gain reduction smoothed in the dB (log) domain, like a log-domain VCA control path
// This is a behavioral model, not a component-level circuit emulation.

#include <algorithm>
#include <cmath>

namespace vcacomp
{
struct Params
{
    float thresholdDb = -12.0f;  // RMS-equivalent level (a full-scale sine reads -3 dB)
    float ratio       = 4.0f;
    float attackMs    = 3.0f;
    float releaseMs   = 400.0f;  // ignored when autoRelease is true
    bool  autoRelease = false;
    float makeupDb    = 0.0f;
    float mix         = 1.0f;    // 0 = dry, 1 = fully compressed
    float scHpfHz     = 60.0f;   // 0 = off
    float kneeDb      = 6.0f;
};

class BusCompressor
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        detRel = coefFor(0.008);          // detector hold/decay ~8 ms (power domain)
        smoothCoef = coefFor(0.010);      // parameter smoothing ~10 ms
        autoFastCoef = coefFor(0.100);    // auto release: fast stage
        autoSlowCoef = coefFor(1.200);    // auto release: slow stage
        reset();
        paramsInit = false;
        lastHpf = -1.0f;
    }

    void reset()
    {
        detEnv = 0.0;
        gr = grFast = grSlow = 0.0;
        hp[0].z1 = hp[0].z2 = hp[1].z1 = hp[1].z2 = 0.0;
        grMeterMax = 0.0f;
    }

    void setParams(const Params& p)
    {
        target = p;
        if (!paramsInit)
        {
            curThr = p.thresholdDb;
            curMakeup = dbToLin(p.makeupDb);
            curMix = p.mix;
            paramsInit = true;
        }
        attackCoef = coefFor(std::max(0.00001, (double) p.attackMs * 0.001));
        releaseCoef = coefFor(std::max(0.001, (double) p.releaseMs * 0.001));

        if (p.scHpfHz != lastHpf)
        {
            lastHpf = p.scHpfHz;
            if (p.scHpfHz > 1.0f)
                setHighpass(p.scHpfHz);
        }
    }

    // In-place stereo processing.
    void process(float* l, float* r, int n)
    {
        float blockMax = 0.0f;
        const bool hpfOn = target.scHpfHz > 1.0f;
        const double knee = target.kneeDb;
        const double ratio = std::max(1.0f, target.ratio);
        const double slope = 1.0 / ratio - 1.0;

        for (int i = 0; i < n; ++i)
        {
            curThr    += (target.thresholdDb - curThr) * (1.0 - smoothCoef);
            curMakeup += (dbToLin(target.makeupDb) - curMakeup) * (1.0 - smoothCoef);
            curMix    += (target.mix - curMix) * (1.0 - smoothCoef);

            const double dl = l[i], dr = r[i];
            double sl = dl, sR = dr;
            if (hpfOn)
            {
                sl = hp[0].process(dl);
                sR = hp[1].process(dr);
            }

            // Stereo-linked power detector: instant attack, short decay (peak-ish, ripple-free)
            const double p = 0.5 * (sl * sl + sR * sR);
            detEnv = p > detEnv ? p : p + (detEnv - p) * detRel;
            const double lvl = 10.0 * std::log10(std::max(detEnv, 1.0e-12)) - 3.0103;

            // Soft-knee static gain computer
            const double over = lvl - curThr;
            double outDb;
            if (knee < 0.01)
                outDb = over > 0.0 ? curThr + over / ratio : lvl;
            else if (2.0 * over < -knee)
                outDb = lvl;
            else if (2.0 * std::fabs(over) <= knee)
            {
                const double t = over + knee * 0.5;
                outDb = lvl + slope * t * t / (2.0 * knee);
            }
            else
                outDb = curThr + over / ratio;

            const double grTarget = std::max(0.0, lvl - outDb);

            // Ballistics on gain reduction in dB
            if (!target.autoRelease)
            {
                const double c = grTarget > gr ? attackCoef : releaseCoef;
                gr = grTarget + c * (gr - grTarget);
            }
            else
            {
                const double cf = grTarget > grFast ? attackCoef : autoFastCoef;
                const double cs = grTarget > grSlow ? attackCoef : autoSlowCoef;
                grFast = grTarget + cf * (grFast - grTarget);
                grSlow = grTarget + cs * (grSlow - grTarget);
                gr = 0.5 * (grFast + grSlow);   // two-stage, program-dependent tail
            }

            const double g = std::exp(-gr * 0.11512925464970229) * curMakeup;
            const double wetL = dl * g, wetR = dr * g;
            l[i] = (float) (dl * (1.0 - curMix) + wetL * curMix);
            r[i] = (float) (dr * (1.0 - curMix) + wetR * curMix);

            blockMax = std::max(blockMax, (float) gr);
        }
        grMeterMax = blockMax;
    }

    float getBlockMaxGainReductionDb() const { return grMeterMax; }
    float getCurrentGainReductionDb() const { return (float) gr; }

private:
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        double process(double x)
        {
            const double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };

    double coefFor(double seconds) const { return std::exp(-1.0 / (seconds * sr)); }
    static double dbToLin(double db) { return std::pow(10.0, db / 20.0); }

    void setHighpass(double f)
    {
        const double w0 = 2.0 * 3.14159265358979323846 * f / sr;
        const double c = std::cos(w0), s = std::sin(w0);
        const double alpha = s / (2.0 * 0.70710678);
        const double a0 = 1.0 + alpha;
        for (auto& b : hp)
        {
            b.b0 = ((1.0 + c) * 0.5) / a0;
            b.b1 = -(1.0 + c) / a0;
            b.b2 = ((1.0 + c) * 0.5) / a0;
            b.a1 = (-2.0 * c) / a0;
            b.a2 = (1.0 - alpha) / a0;
        }
    }

    double sr = 44100.0;
    Params target;
    bool paramsInit = false;
    float lastHpf = -1.0f;

    double detRel = 0, smoothCoef = 0, attackCoef = 0, releaseCoef = 0, autoFastCoef = 0, autoSlowCoef = 0;
    double detEnv = 0, gr = 0, grFast = 0, grSlow = 0;
    double curThr = -12.0, curMakeup = 1.0, curMix = 1.0;
    float grMeterMax = 0.0f;
    Biquad hp[2];
};
} // namespace vcacomp
