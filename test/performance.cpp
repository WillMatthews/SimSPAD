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
#include <vector>
#include <chrono>

#include "../src/sipm.hpp"
#include "../src/utilities.hpp"

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
    vector<double> photonsPerDt = {0, 1, 10, 100, 1000};
    vector<double> expected_runtimes = {240e-12, 240e-12, 275e-12, 550e-12, 3e-9};
    double runtime;
    bool passed = true;

    for (int i = 0; i < (int)photonsPerDt.size(); i++)
    {
        runtime = speed_measure(photonsPerDt[i]);
        cout << photonsPerDt[i] << "\t" << runtime << "\t";
        if (runtime < expected_runtimes[i])
        {
            cout << "PASS" << endl;
        }
        else
        {
            cout << "FAIL" << endl;
            passed = false;
        }
    }
    return passed;
}
