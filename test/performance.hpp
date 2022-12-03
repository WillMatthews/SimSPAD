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
#include <string>
#include <vector>
#include <chrono>

#include "../src/sipm.hpp"
#include "../src/utilities.hpp"

#define BARS 108
#define MACHINE_SPEED_RATIO 2

using namespace std;

double speed_measure(double photonsPerDt)
{
    // create a J30020 SiPM
    SiPM sipm(14410, 27.5, 24.5, 2.2 * 14e-9, 0.0, 4.6e-14, 2.04, 0.46);
    sipm.dt = 2.0e-10;

    int speedTestSamples = 10000;
    vector<double> in(speedTestSamples, photonsPerDt); // DC light source
    vector<double> out = {};

    auto start = chrono::steady_clock::now();
    out = sipm.simulate(in, true);
    auto end = chrono::steady_clock::now();
    chrono::duration<double> elapsed = end - start;

    (void)out;

    double time_per_iter = ((double)elapsed.count()) / ((double)speedTestSamples);
    double time_per_ucell = time_per_iter / sipm.numMicrocell;

    return time_per_ucell;
}

bool TEST_performance()
{
    string BAR_STRING(BARS, '=');
    cout << BAR_STRING << endl;
    cout << "BEGIN TEST: SimSPAD Performance" << endl;
    cout << BAR_STRING << endl;

    vector<double> photonsPerDt = {0, 1, 10, 100, 1000, 10000};
    vector<double> expected_runtimes = {10e-12, 12e-12, 60e-12, 500e-12, 4e-9, 30e-9};
    double runtime;
    bool passed = true;
    bool passed_all = true;
    wstring runtime_prefix;
    double runtime_val;

    for (int i = 0; i < (int)photonsPerDt.size(); i++)
    {
        runtime = speed_measure(photonsPerDt[i]);
        tie(runtime_prefix, runtime_val) = exponent_val(runtime);
        wcout << L"Photons per dt: " << photonsPerDt[i] << L"\t" << runtime_val << runtime_prefix << L"s/(μcell dt)"
              << L"\t";
        passed = (runtime < expected_runtimes[i] * MACHINE_SPEED_RATIO);
        wstring outString = passed ? L"\033[32;49;1mPASS\033[0m" : L"\033[31;49;1mFAIL\033[0m";
        wcout << outString << endl;
        passed_all = passed_all & passed;
    }

    string prefix = passed_all ? "\033[32;49;1m" : "\033[31;49;1m";
    string outStatus = passed_all ? "PASS\n" : "FAIL\a\n";
    cout << prefix << BAR_STRING << endl;
    cout << prefix << "TEST " << outStatus;
    cout << prefix << "END TEST: SimSPAD Performance" << endl;
    cout << prefix << BAR_STRING << "\033[0m" << endl;

    return passed_all;
}
