// spice_kernel: derive the --shape fast kernel from the ngspice equivalent
// circuit (spice/jseries_fastout.cir).
//
// This is the spice -> kernel pipeline. It runs the J-Series fast-output
// equivalent circuit, extracts the single-photon fast-output pulse, and turns
// it into the two time constants the fast/bench shapers consume:
//
//   tauLoad     = R_Lfast * N * C_f   (fast-rail RC, from the datasheet-
//                                      traceable circuit values, read from the
//                                      deck so it cannot drift from the circuit)
//   tauRecovery = loaded recovery constant, fitted on the fast tail
//
// It then checks that the analytic kernel SimSPAD's shape_fast() implements,
//   h(t) = A * ( e^{-t/tauLoad}/tauLoad - e^{-t/tauRecovery}/tauRecovery ),
//   A = tauRecovery / (tauRecovery - tauLoad),
// reproduces the SPICE pulse, and writes a ready-to-run parameter file whose
// tauLoad is calibrated to the circuit, so
//   simspad -p sipm_fast.json -i light.npy -o resp.npy --shape fast
// gives a fast-output trace shaped by the circuit rather than a hand-picked
// time constant.
//
// Prereq: ngspice on PATH (tested with ngspice-45). Run from the repository
// root so the deck's relative wrdata path resolves:
//   make spice_kernel && ./build/apps/spice_kernel

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "sipm.hpp"
#include "utilities.hpp"

using namespace std;

namespace
{
const double T0 = 5e-9; // avalanche trigger time set in the deck (VFIRE ... 5n)

// Parse a SPICE numeric literal with an optional engineering suffix.
double spice_value(string tok)
{
    for (auto &c : tok)
        c = (char)tolower((unsigned char)c);
    // "meg" must be tested before "m"; suffixes are case-folded already.
    static const vector<pair<string, double>> suffix = {
        {"meg", 1e6}, {"f", 1e-15}, {"p", 1e-12}, {"n", 1e-9}, {"u", 1e-6},
        {"m", 1e-3}, {"k", 1e3}, {"g", 1e9}, {"t", 1e12}};
    for (const auto &s : suffix)
    {
        if (tok.size() > s.first.size() &&
            tok.compare(tok.size() - s.first.size(), s.first.size(), s.first) == 0)
        {
            return stod(tok.substr(0, tok.size() - s.first.size())) * s.second;
        }
    }
    return stod(tok);
}

// Pull the `.param NAME=VALUE` lines we need straight from the deck.
map<string, double> read_deck_params(const string &path)
{
    ifstream f(path);
    if (!f)
        throw runtime_error("cannot open deck: " + path);
    map<string, double> p;
    regex re(R"(^\.param\s+(\w+)\s*=\s*\{?([^}\s]+)\}?)", regex::icase);
    string line;
    while (getline(f, line))
    {
        smatch m;
        if (regex_search(line, m, re))
        {
            string name = m[1].str();
            for (auto &c : name)
                c = (char)toupper((unsigned char)c);
            try
            {
                p[name] = spice_value(m[2].str()); // skip braced expressions (e.g. NPAS={NCELL-1})
            }
            catch (const invalid_argument &)
            {
            }
        }
    }
    return p;
}

double fwhm(const vector<double> &t, const vector<double> &y)
{
    double pk = *max_element(y.begin(), y.end());
    size_t lo = 0, hi = 0;
    for (size_t i = 0; i < y.size(); i++)
        if (y[i] >= pk / 2)
        {
            if (!lo)
                lo = i;
            hi = i;
        }
    return t[hi] - t[lo];
}
} // namespace

int main(int argc, char **argv)
{
    const string deck = argc > 1 ? argv[1] : "spice/jseries_fastout.cir";
    const string tran = "spice/fastout_tran.txt";
    const string out_params = argc > 2 ? argv[2] : "sipm_fast.json";

    // 1. Run the circuit.
    const string cmd = "ngspice -b " + deck + " > /dev/null 2>&1";
    if (system(cmd.c_str()) != 0)
    {
        cerr << "ngspice failed (is it on PATH? run from the repo root). cmd: " << cmd << "\n";
        return 2;
    }

    // 2. Read circuit constants from the deck (so this can't drift from it).
    map<string, double> cir = read_deck_params(deck);
    const double CD = cir.at("CD"), CQ = cir.at("CQ"), CF = cir.at("CF");
    const double RLFAST = cir.at("RLFAST"), OV = cir.at("OV");
    const unsigned long NCELL = (unsigned long)llround(cir.at("NCELL"));

    // 3. Read the transient (wrdata columns: t v(f) t v(a)); fast current = v(f)/R.
    ifstream tf(tran);
    if (!tf)
    {
        cerr << "cannot open transient output: " << tran << "\n";
        return 2;
    }
    vector<double> t, i_f;
    {
        double a, b, c, d;
        while (tf >> a >> b >> c >> d)
        {
            t.push_back(a);
            i_f.push_back(b / RLFAST);
        }
    }
    if (t.size() < 16)
    {
        cerr << "transient too short (" << t.size() << " rows)\n";
        return 2;
    }

    // 4. Derive the kernel constants.
    const double tau_load = RLFAST * (double)NCELL * CF; // fast-rail RC, from the circuit
    const double kappa = CF / (CD + CQ);                 // fast/avalanche charge fraction
    // Loaded recovery constant: log-linear least squares on the negative tail,
    // well clear of the fast edge.
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    long n = 0;
    for (size_t k = 0; k < t.size(); k++)
    {
        if (t[k] > T0 + 15e-9 && t[k] < T0 + 75e-9 && i_f[k] < 0)
        {
            double x = t[k], y = log(-i_f[k]);
            sx += x;
            sy += y;
            sxx += x * x;
            sxy += x * y;
            n++;
        }
    }
    const double slope = (n * sxy - sx * sy) / (n * sxx - sx * sx);
    const double tau_rec = -1.0 / slope;

    // 5. Validate the analytic kernel against the SPICE pulse (peak-normalised
    //    shape, so we test the shape the device makes, not the gain).
    const double A = tau_rec / (tau_rec - tau_load);
    vector<double> tt, hk, sp;
    for (size_t k = 0; k < t.size(); k++)
    {
        double u = t[k] - T0;
        if (u < 0)
            continue;
        tt.push_back(u);
        hk.push_back(A * (exp(-u / tau_load) / tau_load - exp(-u / tau_rec) / tau_rec));
        sp.push_back(i_f[k]);
    }
    const double hk_pk = *max_element(hk.begin(), hk.end());
    const double sp_pk = *max_element(sp.begin(), sp.end());
    double se = 0;
    long m = 0;
    for (size_t k = 0; k < tt.size(); k++)
        if (tt[k] < 40e-9)
        {
            double e = hk[k] / hk_pk - sp[k] / sp_pk;
            se += e * e;
            m++;
        }
    const double rms = sqrt(se / m);
    const double fwhm_spice = fwhm(t, i_f), fwhm_kernel = fwhm(tt, hk);

    printf("=== spice -> --shape fast kernel ===\n");
    printf("  circuit: N=%lu, C_f=%.2f fF, R_Lfast=%.0f ohm, OV=%.1f V\n",
           NCELL, CF * 1e15, RLFAST, OV);
    printf("  tauLoad     = R_Lfast*N*C_f = %7.3f ns   (-> params 'tauLoad')\n", tau_load * 1e9);
    printf("  tauRecovery = loaded fit    = %7.3f ns   (fast-tail recovery)\n", tau_rec * 1e9);
    printf("  kappa       = C_f/(C_d+C_q) = %.4e\n", kappa);
    printf("  FWHM: spice %.2f ns vs analytic kernel %.2f ns\n", fwhm_spice * 1e9, fwhm_kernel * 1e9);
    printf("  peak-normalised shape RMS error (kernel vs spice, <40 ns): %.3f\n", rms);
    const double tol = 0.10;
    const bool ok = rms < tol;
    printf("  [%s] shape RMS %.3f %s %.2f\n", ok ? "PASS" : "FAIL", rms, ok ? "<" : ">=", tol);

    // 6. Emit a ready-to-run J30020 parameter file with the circuit-derived
    //    tauLoad (remaining device parameters match examples/python/example.py).
    vector<double> svars = {
        1e-11,           // dt
        (double)NCELL,   // numMicrocell
        OV + 24.5,       // vBias = OV + vBr
        24.5,            // vBr
        2.2 * 14e-9,     // tauRecovery (device)
        0.46,            // pdeMax
        2.04,            // vChr
        4.6e-14,         // cCell
        1.5e-9,          // tauFwhm
        0.0};            // digitalThreshold
    SiPM sipm(svars);
    sipm.tauLoad = tau_load;
    save_params_json(out_params, sipm);
    printf("  wrote %s (run: simspad -p %s -i light.npy -o resp.npy --shape fast)\n",
           out_params.c_str(), out_params.c_str());

    return ok ? 0 : 1;
}
