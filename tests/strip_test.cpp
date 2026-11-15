// Measured tests for the channel strip DSP. Prints real numbers; exits non-zero on failure.
#include "../Source/ChannelStrip.h"
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

using namespace strip;
static int fails = 0;
static void check(bool ok, const char* name, const char* fmt = "", double a = 0, double b = 0, double c = 0)
{
    char buf[200]; std::snprintf(buf, sizeof buf, fmt, a, b, c);
    std::printf("[%s] %-46s %s\n", ok ? "PASS" : "FAIL", name, buf);
    if (!ok) ++fails;
}
static double db(double x) { return 20 * std::log10(std::max(x, 1e-30)); }

struct Run { std::vector<float> l, r; };

static Run render(const Params& p, const std::vector<float>& inL, const std::vector<float>& inR, double sr, int block = 256)
{
    ChannelStrip s; s.prepare(sr); s.setParams(p);
    Run o; o.l = inL; o.r = inR;
    for (size_t i = 0; i < inL.size(); i += block)
    {
        const int n = (int) std::min<size_t>(block, inL.size() - i);
        s.setParams(p);
        s.process(o.l.data() + i, o.r.data() + i, n);
    }
    return o;
}
static std::vector<float> sine(double f, double amp, int n, double sr)
{
    std::vector<float> v(n);
    for (int i = 0; i < n; ++i) v[i] = (float) (amp * std::sin(2 * kPi * f * i / sr));
    return v;
}
// amplitude of frequency f over the last `len` samples (len must hold an integer number of periods)
static double tone(const std::vector<float>& x, double f, double sr, int len)
{
    double re = 0, im = 0; const int s = (int) x.size() - len;
    for (int i = 0; i < len; ++i) { const double w = 2 * kPi * f * i / sr; re += x[s + i] * std::cos(w); im += x[s + i] * std::sin(w); }
    return 2 * std::sqrt(re * re + im * im) / len;
}
static double rmsOfResidual(const std::vector<float>& x, double f, double sr, int len, double* thdOut = nullptr)
{
    // remove the fundamental, return RMS of everything else (THD+N)
    double re = 0, im = 0; const int s = (int) x.size() - len;
    for (int i = 0; i < len; ++i) { const double w = 2 * kPi * f * i / sr; re += x[s + i] * std::cos(w); im += x[s + i] * std::sin(w); }
    re *= 2.0 / len; im *= 2.0 / len;
    double e = 0, sig = 0;
    for (int i = 0; i < len; ++i)
    {
        const double w = 2 * kPi * f * i / sr, fit = re * std::cos(w) + im * std::sin(w);
        const double d = x[s + i] - fit; e += d * d; sig += fit * fit;
    }
    if (thdOut) *thdOut = std::sqrt(sig / len);
    return std::sqrt(e / len);
}

int main()
{
    const double sr = 48000;
    const int N = 48000 * 2, LEN = 48000;
    std::vector<float> zero(N, 0.0f);

    // ---- 1. latency, constant at every sample rate
    for (double r : { 44100.0, 48000.0, 96000.0 })
    {
        std::vector<float> imp(4096, 0.0f); imp[100] = 1.0f;
        Params p; auto o = render(p, imp, zero.size() >= 4096 ? std::vector<float>(4096, 0.0f) : imp, r);
        int pk = 0; for (int i = 0; i < 4096; ++i) if (std::fabs(o.l[i]) > std::fabs(o.l[pk])) pk = i;
        check(pk - 100 == kLatencySamples, "latency (impulse peak delay)", "%.0f samples @ %.0f Hz", pk - 100, r);
    }

    // ---- 2. neutral path flatness 40 Hz..18 kHz
    {
        double worst = 0, wf = 0;
        for (double f : { 40.0, 100.0, 1000.0, 5000.0, 10000.0, 16000.0, 18000.0 })
        {
            Params p; auto in = sine(f, 0.5, N, sr); auto o = render(p, in, zero, sr);
            const double g = db(tone(o.l, f, sr, LEN) / 0.5);
            if (std::fabs(g) > std::fabs(worst)) { worst = g; wf = f; }
        }
        check(std::fabs(worst) < 0.05, "neutral flatness 40Hz-18kHz", "worst %.4f dB at %.0f Hz", worst, wf);
    }

    // ---- 3. headroom: +24 dBFS in (amplitude 15.85), everything neutral
    {
        const double amp = std::pow(10.0, 24.0 / 20);
        Params p; auto in = sine(1000, amp, N, sr); auto o = render(p, in, zero, sr);
        double thd; const double e = rmsOfResidual(o.l, 1000, sr, LEN, &thd);
        const double g = db(tone(o.l, 1000, sr, LEN) / amp);
        check(std::fabs(g) < 0.01 && db(e / thd) < -100, "headroom +24 dBFS clean", "gain %.4f dB, THD+N %.1f dB", g, db(e / thd));
    }

    // ---- 4. preamp: 60 and 72 dB of clean gain
    for (double gdb : { 60.0, 72.0 })
    {
        const double amp = std::pow(10.0, (-80.0 - (gdb - 60.0) - 10.0 + 10.0) / 20); // -80 dBFS @60, -92 @72
        Params p; p.preGainDb = (float) gdb;
        auto in = sine(1000, amp, N, sr); auto o = render(p, in, zero, sr);
        double sigRms; const double e = rmsOfResidual(o.l, 1000, sr, LEN, &sigRms);
        const double g = db(tone(o.l, 1000, sr, LEN) / amp);
        check(std::fabs(g - gdb) < 0.02 && db(e / sigRms) < -90, "preamp clean gain", "gain %.3f dB (want %.0f), THD+N %.1f dB", g, gdb, db(e / sigRms));
    }

    // ---- 5. noise floor
    {
        Params p; p.preGainDb = 72; p.eqOn = true; p.lmfGainDb = 6; p.satAmount = 0.5f; p.compOn = true;
        auto o = render(p, zero, zero, sr);
        float mx = 0; for (float v : o.l) mx = std::max(mx, std::fabs(v));
        for (float v : o.r) mx = std::max(mx, std::fabs(v));
        check(mx == 0.0f, "silence in -> silence out (all stages, 72 dB)", "max |out| = %.3g", mx);
        // added noise on a very quiet tone: residual vs ideal linear result at the 0 dB neutral setting
        Params n0; auto in = sine(997, 1e-6, N, sr); auto oo = render(n0, in, zero, sr);   // -120 dBFS
        double sig; const double e = rmsOfResidual(oo.l, 997, sr, LEN, &sig);
        check(db(e) < -135, "added noise, -120 dBFS tone", "residual %.1f dBFS", db(e));
    }

    // ---- 6. crosstalk: hard left signal, every stage engaged; right must stay silent
    {
        Params p; p.preGainDb = 20; p.satAmount = 0.6f; p.satMode = ClassA; p.hpfHz = 80; p.lpfHz = 14000;
        p.gateOn = true; p.gateThrDb = -60; p.compOn = true; p.compThrDb = -30; p.lfGainDb = 8; p.hmfGainDb = -6; p.hfGainDb = 6; p.pan = -0.3f;
        auto in = sine(1000, 0.3, N, sr); auto o = render(p, in, zero, sr);
        float mx = 0; for (float v : o.r) mx = std::max(mx, std::fabs(v));
        const double sigL = tone(o.l, 1000, sr, LEN);
        check(mx < 1e-9f, "channel separation (L only -> R)", "R peak %.3g (%.0f dB below L)", mx, db(sigL) - db(mx + 1e-30));
    }

    // ---- 7. EQ accuracy and musicality
    {
        struct T { const char* n; float Params::*gain; double f; double want; };
        auto meas = [&](Params p, double f) { auto in = sine(f, 0.1, N, sr); auto o = render(p, in, zero, sr); return db(tone(o.l, f, sr, LEN) / 0.1); };
        Params p;
        p = Params(); p.lmfGainDb = 8; p.lmfFreq = 1000; check(std::fabs(meas(p, 1000) - 8) < 0.1, "LMF +8 dB at centre", "%.3f dB", meas(p, 1000));
        p = Params(); p.hmfGainDb = -8; p.hmfFreq = 3000; check(std::fabs(meas(p, 3000) + 8) < 0.1, "HMF -8 dB at centre", "%.3f dB", meas(p, 3000));
        p = Params(); p.lfGainDb = 10; p.lfFreq = 100; double a = meas(p, 30); check(std::fabs(a - 10) < 0.6, "LF shelf +10 dB (30 Hz plateau)", "%.3f dB", a);
        p = Params(); p.hfGainDb = 8; p.hfFreq = 8000; a = meas(p, 18000); check(std::fabs(a - 8) < 0.9, "HF shelf +8 dB (18 kHz plateau)", "%.3f dB", a);
        // boost and cut are exact mirror images
        double worst = 0;
        for (double f = 40; f < 18000; f *= 1.07)
        {
            const double up = magnitude(peakCoeffs(sr, 1200, 9, 1.0), sr, f), dn = magnitude(peakCoeffs(sr, 1200, -9, 1.0), sr, f);
            worst = std::max(worst, std::fabs(db(up * dn)));
        }
        check(worst < 1e-9, "boost then cut cancels exactly", "worst %.2e dB", worst);
        // shelves: monotonic response (no resonant bump), the "smooth" requirement
        bool mono = true; double prev = magnitude(lowShelfCoeffs(sr, 100, 12, 0.6), sr, 20);
        for (double f = 22; f < 2000; f *= 1.05) { const double m = magnitude(lowShelfCoeffs(sr, 100, 12, 0.6), sr, f); if (m > prev + 1e-9) mono = false; prev = m; }
        double mx = 0; for (double f = 20; f < 20000; f *= 1.05) mx = std::max(mx, db(magnitude(highShelfCoeffs(sr, 8000, 12, 0.6), sr, f)));
        check(mono && mx < 12.05, "shelf overshoot", "LF monotonic, HF peak %.3f dB (boost 12)", mx);
        // proportional Q: bandwidth widens as gain gets smaller
        Params q; q.eqPropQ = true; q.lmfGainDb = 3; auto c3 = ChannelStrip::designBand(q, 1, sr); q.lmfGainDb = 15; auto c15 = ChannelStrip::designBand(q, 1, sr);
        auto bw = [&](const Coeffs& k, double g) { double lo = 0, hi = 0; for (double f = 100; f < 6000; f *= 1.005) { if (db(magnitude(k, sr, f)) > g / 2) { if (!lo) lo = f; hi = f; } } return hi - lo; };
        check(bw(c3, 3) > bw(c15, 15), "prop-Q: small boosts are broader", "3 dB: %.0f Hz wide, 15 dB: %.0f Hz wide", bw(c3, 3), bw(c15, 15));
    }

    // ---- 8. saturation character
    {
        auto harm = [&](int mode, double f, double amt) {
            Params p; p.satMode = mode; p.satAmount = (float) amt;
            auto in = sine(f, 0.5, N, sr); auto o = render(p, in, zero, sr);
            const double f1 = tone(o.l, f, sr, LEN); return std::make_pair(db(tone(o.l, 2 * f, sr, LEN) / f1), db(tone(o.l, 3 * f, sr, LEN) / f1)); };
        auto x100 = harm(Transformer, 100, 0.5), x1k = harm(Transformer, 1000, 0.5), ca = harm(ClassA, 1000, 0.5);
        check(x100.second > x1k.second + 3, "transformer: more 3rd at LF than at 1k", "H3 %.1f dB @100Hz vs %.1f dB @1k", x100.second, x1k.second);
        check(x1k.second > x1k.first + 10, "transformer: odd harmonics dominate", "H2 %.1f, H3 %.1f dB", x1k.first, x1k.second);
        check(ca.first > ca.second, "class-A: 2nd harmonic leads", "H2 %.1f, H3 %.1f dB", ca.first, ca.second);
        auto lo = harm(Transformer, 1000, 0.1);
        check(lo.second < -50, "low drive stays subtle", "H3 %.1f dB at drive 0.1", lo.second);
    }

    // ---- 9. aliasing: 15 kHz at drive 1, alias products vs naive 1x tanh
    {
        const double f = 15000; const int n = 96000; std::vector<float> in = sine(f, 0.7, n, sr);
        Params p; p.satAmount = 1.0f; auto o = render(p, in, zero, sr);
        // 3*15k = 45k aliases to 3k; 2*15k->18k is a real in-band harmonic (not alias) for even, 4th -> 12k
        const double fund = tone(o.l, f, sr, LEN);
        const double aliasOs = db(tone(o.l, 3000, sr, LEN) / fund);
        std::vector<float> naive(n); for (int i = 0; i < n; ++i) naive[i] = Shaper::tanhUnity(in[i], 2.5f * 3.0f * 0.0f + 2.5f);
        const double aliasNaive = db(tone(naive, 3000, sr, LEN) / tone(naive, f, sr, LEN));
        check(aliasOs < aliasNaive - 25, "aliasing vs non-oversampled", "3 kHz alias %.1f dB (naive %.1f dB)", aliasOs, aliasNaive);
    }

    // ---- 10. gate
    {
        Params p; p.gateOn = true; p.gateThrDb = -40; p.gateRangeDb = -40; p.gateHoldMs = 10; p.gateReleaseMs = 50;
        auto lowIn = sine(1000, std::pow(10.0, -50 / 20.0), N, sr), hiIn = sine(1000, std::pow(10.0, -20 / 20.0), N, sr);
        auto a = render(p, lowIn, zero, sr), b = render(p, hiIn, zero, sr);
        const double ga = db(tone(a.l, 1000, sr, LEN) / std::pow(10.0, -50 / 20.0)), gb = db(tone(b.l, 1000, sr, LEN) / std::pow(10.0, -20 / 20.0));
        check(std::fabs(ga + 40) < 1.0 && std::fabs(gb) < 0.1, "gate closes below / passes above threshold", "below %.2f dB, above %.3f dB", ga, gb);
    }

    // ---- 11. compressor
    {
        Params p; p.compOn = true; p.compThrDb = -21; p.compRatio = 4; p.compAttackMs = 5; p.compReleaseMs = 100;
        auto in = sine(1000, 0.5, N, sr); auto o = render(p, in, in, sr);
        const double out = db(tone(o.l, 1000, sr, LEN)), expect = -6 - 9;
        check(std::fabs(out - expect) < 0.7, "compressor 4:1, 12 dB over threshold", "out %.2f dBFS (theory %.1f)", out, expect);
        p.compPostEq = true; auto o2 = render(p, in, in, sr);
        check(std::fabs(db(tone(o2.l, 1000, sr, LEN)) - expect) < 0.7, "compressor post-EQ position works", "out %.2f dBFS", db(tone(o2.l, 1000, sr, LEN)));
    }

    // ---- 12. fader / balance / polarity
    {
        auto in = sine(1000, 0.5, N, sr);
        Params p; p.outDb = -6; auto o = render(p, in, in, sr);
        check(std::fabs(db(tone(o.l, 1000, sr, LEN) / 0.5) + 6) < 0.02, "fader -6 dB", "%.3f dB", db(tone(o.l, 1000, sr, LEN) / 0.5));
        p = Params(); p.pan = 1.0f; o = render(p, in, in, sr);
        float mx = 0; for (float v : o.l) mx = std::max(mx, std::fabs(v));
        check(mx < 1e-6f && tone(o.r, 1000, sr, LEN) > 0.49, "balance hard right silences left", "L peak %.2g", mx);
        p = Params(); p.polarity = true; o = render(p, in, in, sr);
        auto o0 = render(Params(), in, in, sr); double dot = 0; for (int i = N - LEN; i < N; ++i) dot += o.l[i] * o0.l[i];
        check(dot < 0, "polarity invert", "correlation with normal %.3f", dot / LEN);
    }

    // ---- 13. stability: noise through everything, odd block sizes
    {
        std::mt19937 rng(1); std::uniform_real_distribution<float> d(-1, 1);
        std::vector<float> a(N), b(N); for (auto& v : a) v = d(rng); for (auto& v : b) v = d(rng);
        Params p; p.preGainDb = 24; p.satAmount = 1; p.satMode = ClassA; p.hpfHz = 40; p.lpfHz = 18000; p.gateOn = true; p.compOn = true;
        p.lfGainDb = 14; p.lmfGainDb = 12; p.hmfGainDb = -12; p.hfGainDb = 14; p.eqPropQ = true; p.pan = 0.4f;
        auto o = render(p, a, b, sr, 77);
        bool ok = true; float pk = 0; for (size_t i = 0; i < o.l.size(); ++i) { if (!std::isfinite(o.l[i]) || !std::isfinite(o.r[i])) ok = false; pk = std::max(pk, std::fabs(o.l[i])); }
        check(ok && pk < 100, "stable under extreme settings", "peak %.2f", pk);
    }

    std::printf("\n%s (%d failure%s)\n", fails ? "SOME TESTS FAILED" : "ALL TESTS PASSED", fails, fails == 1 ? "" : "s");
    return fails ? 1 : 0;
}
