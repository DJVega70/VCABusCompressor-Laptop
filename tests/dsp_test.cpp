#include "../Source/BusCompressor.h"
#include <cstdio>
#include <vector>
#include <cmath>
using namespace vcacomp;

static int fails = 0;
static void check(const char* name, double got, double want, double tol)
{
    bool ok = std::fabs(got - want) <= tol;
    std::printf("%-46s got %8.3f  want %8.3f  (+/-%.2f)  %s\n", name, got, want, tol, ok ? "OK" : "FAIL");
    if (!ok) ++fails;
}

static double runSine(Params p, double amp, double seconds, double sr = 48000.0, double freq = 1000.0)
{
    BusCompressor c; c.prepare(sr); c.setParams(p);
    const int n = (int) (seconds * sr);
    std::vector<float> l(n), r(n);
    for (int i = 0; i < n; ++i) l[i] = r[i] = (float) (amp * std::sin(2.0 * M_PI * freq * i / sr));
    c.process(l.data(), r.data(), n);
    return c.getCurrentGainReductionDb();
}

int main()
{
    Params p; p.thresholdDb = -20; p.ratio = 4; p.attackMs = 1; p.releaseMs = 400; p.scHpfHz = 0; p.kneeDb = 6;
    // -6 dBFS sine = -9.03 dB RMS-equivalent -> over = 10.97 dB
    check("GR 4:1, 1 kHz sine at -6 dBFS", runSine(p, 0.5, 3.0), 10.97 * 0.75, 0.3);
    p.ratio = 10;
    check("GR 10:1, same signal", runSine(p, 0.5, 3.0), 10.97 * 0.90, 0.3);
    p.ratio = 2;
    check("GR 2:1, same signal", runSine(p, 0.5, 3.0), 10.97 * 0.50, 0.3);
    p.ratio = 4;
    check("No GR well below threshold (-40 dBFS)", runSine(p, 0.01, 2.0), 0.0, 0.05);

    // Mix = 0 -> bit-transparent passthrough
    {
        Params q = p; q.mix = 0.0f; q.makeupDb = 0;
        BusCompressor c; c.prepare(48000); c.setParams(q);
        std::vector<float> l(4800), r(4800), l0, r0;
        for (int i = 0; i < 4800; ++i) l[i] = r[i] = (float) (0.9 * std::sin(i * 0.05));
        l0 = l; r0 = r;
        c.process(l.data(), r.data(), 4800);
        double maxd = 0; for (int i = 0; i < 4800; ++i) maxd = std::max(maxd, (double) std::fabs(l[i] - l0[i]));
        check("mix=0 passthrough max diff", maxd, 0.0, 1e-6);
    }

    // Make-up gain below threshold: +6 dB -> x2 amplitude
    {
        Params q = p; q.makeupDb = 6.0f; q.thresholdDb = 0;
        BusCompressor c; c.prepare(48000); c.setParams(q);
        std::vector<float> l(48000), r(48000);
        for (int i = 0; i < 48000; ++i) l[i] = r[i] = (float) (0.05 * std::sin(i * 0.1));
        c.process(l.data(), r.data(), 48000);
        double pk = 0; for (int i = 40000; i < 48000; ++i) pk = std::max(pk, (double) std::fabs(l[i]));
        check("make-up +6 dB amplitude", pk, 0.05 * 1.9953, 0.002);
    }

    // Attack: 30 ms vs 0.1 ms -> GR after 5 ms
    {
        auto grAfter = [&](float atk) {
            Params q = p; q.attackMs = atk; q.thresholdDb = -30; q.ratio = 10;
            BusCompressor c; c.prepare(48000); c.setParams(q);
            int n = 240; std::vector<float> l(n), r(n);
            for (int i = 0; i < n; ++i) l[i] = r[i] = (float) (0.7 * std::sin(2 * M_PI * 1000.0 * i / 48000.0));
            c.process(l.data(), r.data(), n); return (double) c.getCurrentGainReductionDb();
        };
        double fast = grAfter(0.1f), slow = grAfter(30.f);
        std::printf("GR after 5 ms: attack 0.1 ms = %.2f dB, attack 30 ms = %.2f dB\n", fast, slow);
        if (!(fast > 3.0 * slow)) { std::printf("FAIL attack ordering\n"); ++fails; }
    }

    // Release time constant: burst then quiet; time to fall to 37% of GR
    auto relTime = [&](float relMs, bool autoRel) {
        Params q = p; q.releaseMs = relMs; q.autoRelease = autoRel; q.thresholdDb = -30; q.ratio = 10; q.attackMs = 0.1f;
        const double sr = 48000; BusCompressor c; c.prepare(sr); c.setParams(q);
        int n = (int) (6.0 * sr); std::vector<float> l(n), r(n);
        for (int i = 0; i < n; ++i) { double a = i < (int) (1.5 * sr) ? 0.7 : 0.0005; l[i] = r[i] = (float) (a * std::sin(2 * M_PI * 1000.0 * i / sr)); }
        int split = (int) (1.5 * sr);
        c.process(l.data(), r.data(), split);
        double g0 = c.getCurrentGainReductionDb();
        double t = -1;
        for (int i = 0; i < n - split; ++i) { float a = l[split + i], b = r[split + i]; c.process(&a, &b, 1); if (c.getCurrentGainReductionDb() < g0 * 0.3679) { t = i / sr; break; } }
        return t * 1000.0;
    };
    check("Release 100 ms -> 37% time (ms)", relTime(100, false), 100.0, 25.0);
    check("Release 400 ms -> 37% time (ms)", relTime(400, false), 400.0, 80.0);
    check("Release 1200 ms -> 37% time (ms)", relTime(1200, false), 1200.0, 250.0);
    double a = relTime(0, true);
    std::printf("Auto release 37%% time: %.0f ms (two-stage, fast ~100 ms then slow ~1.2 s)\n", a);

    // Sidechain HPF: low 30 Hz tone should compress less with 150 Hz HPF than without
    {
        Params q = p; q.thresholdDb = -30; q.ratio = 10; q.scHpfHz = 0;
        double off = runSine(q, 0.5, 2.0, 48000, 30.0);
        q.scHpfHz = 150;
        double on = runSine(q, 0.5, 2.0, 48000, 30.0);
        std::printf("30 Hz tone GR: HPF off %.2f dB, HPF 150 Hz %.2f dB\n", off, on);
        if (!(on < off - 3.0)) { std::printf("FAIL hpf\n"); ++fails; }
    }
    std::printf("\n%s\n", fails ? "SOME TESTS FAILED" : "ALL TESTS PASSED");
    return fails ? 1 : 0;
}
