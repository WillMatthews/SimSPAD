/*
 * This file is part of the SimSPAD distribution (http://github.com/WillMatthews/SimSPAD).
 * Copyright (c) 2022 William Matthews.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>

#include "../src/sipm.hpp"
#include "../src/utilities.hpp"

#define BARS 102

using namespace std;

// Measure the FWHM of a discrete kernel (in samples) by linearly
// interpolating the half-maximum crossings either side of the peak.
static double kernel_fwhm_samples(const vector<double> &kernel)
{
    int peak = 0;
    for (int i = 0; i < (int)kernel.size(); i++)
    {
        if (kernel[i] > kernel[peak])
        {
            peak = i;
        }
    }
    double half = kernel[peak] / 2.0;

    // walk left from the peak to the half-maximum crossing
    double left = 0.0;
    for (int i = peak; i > 0; i--)
    {
        if (kernel[i - 1] < half)
        {
            left = (double)(i - 1) + (half - kernel[i - 1]) / (kernel[i] - kernel[i - 1]);
            break;
        }
    }
    // walk right from the peak to the half-maximum crossing
    double right = (double)kernel.size() - 1;
    for (int i = peak; i < (int)kernel.size() - 1; i++)
    {
        if (kernel[i + 1] < half)
        {
            right = (double)i + (kernel[i] - half) / (kernel[i] - kernel[i + 1]);
            break;
        }
    }
    return right - left;
}

// Pulse-shaping unit tests: Gaussian kernel correctness (normalisation and
// FWHM) and the bipolar fast-output shaper (zero net charge, agreement with
// the analytic impulse response).
bool TEST_shaping()
{
    string BAR_STRING(BARS, '=');
    cout << BAR_STRING << endl;
    cout << "BEGIN TEST: Output Pulse Shaping" << endl;
    cout << BAR_STRING << endl;

    bool passed_all = true;
    bool passed;
    cout << setprecision(6);

    const double dt = 5e-11;
    const double tauFwhm = 1.5e-9;
    const double tauRecovery = 30.8e-9;
    const double tauLoad = 2.0e-9;

    // --- 1. Gaussian kernel: discrete sum exactly 1 (charge conservation) ---
    vector<double> kernel = get_gaussian(dt, tauFwhm);
    double sum = 0.0;
    for (int i = 0; i < (int)kernel.size(); i++)
    {
        sum += kernel[i];
    }
    passed = fabs(sum - 1.0) < 1e-12;
    cout << "Gaussian kernel sum:          " << sum << " (want 1 +- 1e-12)\t\t"
         << (passed ? "\033[32;49;1mPASS\033[0m" : "\033[31;49;1mFAIL\033[0m") << endl;
    passed_all = passed_all & passed;

    // --- 2. Gaussian kernel: symmetric, and FWHM within 5% of request ---
    passed = (kernel.size() % 2 == 1);
    for (int i = 0; i < (int)kernel.size() / 2; i++)
    {
        passed = passed && (kernel[i] == kernel[kernel.size() - 1 - i]);
    }
    cout << "Gaussian kernel symmetry:     " << kernel.size() << " points (odd, mirrored)\t\t"
         << (passed ? "\033[32;49;1mPASS\033[0m" : "\033[31;49;1mFAIL\033[0m") << endl;
    passed_all = passed_all & passed;

    double fwhmMeasured = kernel_fwhm_samples(kernel) * dt;
    passed = fabs(fwhmMeasured / tauFwhm - 1.0) < 0.05;
    cout << "Gaussian kernel FWHM:         " << fwhmMeasured << " s (want " << tauFwhm
         << " s +- 5%)\t"
         << (passed ? "\033[32;49;1mPASS\033[0m" : "\033[31;49;1mFAIL\033[0m") << endl;
    passed_all = passed_all & passed;

    // --- 3. Degenerate Gaussian widths collapse to the identity kernel ---
    vector<double> identity = get_gaussian(dt, 0.0);
    passed = (identity.size() == 1) && (identity[0] == 1.0);
    identity = get_gaussian(dt, -1.0);
    passed = passed && (identity.size() == 1) && (identity[0] == 1.0);
    cout << "Degenerate width -> identity: {1.0}\t\t\t\t\t"
         << (passed ? "\033[32;49;1mPASS\033[0m" : "\033[31;49;1mFAIL\033[0m") << endl;
    passed_all = passed_all & passed;

    // --- 4. conv1d with the identity kernel is a no-op (no time shift) ---
    {
        vector<double> probe = {0.0, 1.0, 2.0, 3.0, 0.5, 0.0};
        vector<double> conv = conv1d(probe, {1.0});
        passed = (conv.size() == probe.size());
        for (int i = 0; passed && i < (int)probe.size(); i++)
        {
            passed = passed && (conv[i] == probe[i]);
        }
        cout << "conv1d identity no-op:        same length, bitwise equal\t\t"
             << (passed ? "\033[32;49;1mPASS\033[0m" : "\033[31;49;1mFAIL\033[0m") << endl;
        passed_all = passed_all & passed;
    }

    // --- 5. conv1d conserves charge away from the edges ---
    {
        vector<double> impulse(2048, 0.0);
        impulse[1024] = 1.0; // well clear of both edges
        vector<double> conv = conv1d(impulse, kernel);
        double convSum = 0.0;
        for (int i = 0; i < (int)conv.size(); i++)
        {
            convSum += conv[i];
        }
        passed = (conv.size() == impulse.size()) && (fabs(convSum - 1.0) < 1e-12);
        cout << "conv1d charge conservation:   " << convSum << " (want 1 +- 1e-12)\t\t"
             << (passed ? "\033[32;49;1mPASS\033[0m" : "\033[31;49;1mFAIL\033[0m") << endl;
        passed_all = passed_all & passed;
    }

    // Device under test for the fast shaper (J30035-like; only dt,
    // tauRecovery and tauLoad matter to the shaper).
    SiPM sipm(5676, 27.5, 24.5, tauRecovery, 0.0, 14e-15, 2.04, 0.46);
    sipm.dt = dt;
    sipm.tauLoad = tauLoad;

    // --- 6. Fast shaping: zero net charge (AC coupling) over a long window ---
    const int n = 60000; // 3 us >> 5*tauRecovery
    vector<double> q(n, 0.0);
    q[0] = 1.0; // unit charge impulse
    vector<double> shaped = sipm.shape_fast(q);
    double net = 0.0;
    for (int i = 0; i < n; i++)
    {
        net += shaped[i];
    }
    passed = ((int)shaped.size() == n) && (fabs(net) < 1e-3);
    cout << "Fast shaper net charge:       " << net << " (want |.| < 1e-3)\t"
         << (passed ? "\033[32;49;1mPASS\033[0m" : "\033[31;49;1mFAIL\033[0m") << endl;
    passed_all = passed_all & passed;

    // --- 7. Fast shaping: matches the analytic bipolar impulse response ---
    // h(t) = A * (e^{-t/tauL}/tauL - e^{-t/tauR}/tauR), A = tauR/(tauR-tauL).
    // The discrete one-pole bins the response, so compare each output sample
    // against h evaluated at the bin midpoint, scaled by dt (charge per step).
    {
        const double A = tauRecovery / (tauRecovery - tauLoad);
        double hMax = 0.0, errMax = 0.0;
        for (int i = 0; i < n; i++)
        {
            double t = (i + 0.5) * dt; // bin midpoint
            double h = A * (exp(-t / tauLoad) / tauLoad - exp(-t / tauRecovery) / tauRecovery) * dt;
            hMax = max(hMax, fabs(h));
            errMax = max(errMax, fabs(shaped[i] - h));
        }
        double relErr = errMax / hMax;
        passed = relErr < 5e-3;
        cout << "Fast shaper vs analytic h(t): max rel err " << relErr << " (want < 5e-3)\t"
             << (passed ? "\033[32;49;1mPASS\033[0m" : "\033[31;49;1mFAIL\033[0m") << endl;
        passed_all = passed_all & passed;
    }

    // --- 8. Chunked fast shaping identical to one-shot (streaming state) ---
    {
        vector<double> chunked(n, 0.0);
        sipm.reset_fast_shaper();
        const int chunk = 777; // deliberately awkward chunk size
        for (int off = 0; off < n; off += chunk)
        {
            int m = min(chunk, n - off);
            sipm.shape_fast_chunk(q.data() + off, chunked.data() + off, (size_t)m);
        }
        passed = true;
        for (int i = 0; i < n; i++)
        {
            passed = passed && (chunked[i] == shaped[i]);
        }
        cout << "Chunked == one-shot shaping:  bitwise equal across chunk joins\t\t"
             << (passed ? "\033[32;49;1mPASS\033[0m" : "\033[31;49;1mFAIL\033[0m") << endl;
        passed_all = passed_all & passed;
    }

    string prefix = passed_all ? "\033[32;49;1m" : "\033[31;49;1m";
    string outStatus = passed_all ? "PASS\n" : "FAIL\a\n";
    cout << prefix << BAR_STRING << endl;
    cout << prefix << "TEST " << outStatus;
    cout << prefix << "END TEST: Output Pulse Shaping" << endl;
    cout << prefix << BAR_STRING << "\033[0m" << endl;
    return passed_all;
}
