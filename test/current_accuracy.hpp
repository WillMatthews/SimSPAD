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
#include <string>
#include <tuple>
#include <cmath>

#include "../src/sipm.hpp"
#include "../src/utilities.hpp"
#include "../src/constants.hpp"

#define BARS 108

using namespace std;

double ibias_check(SiPM sipm, double photonsPerDt)
{
    int testSamples = 20000;
    vector<double> in(testSamples, photonsPerDt); // DC light source

    vector<double> out = {};

    bool silence = true;
    out = sipm.simulate(in, in, silence);

    // throw out first 10 RC times as statistics have not stabilised
    int discard = (int)10 * (sipm.tauRecovery / sipm.dt);

    double sumOut = 0;
    for (int i = discard; i < (int)out.size(); i++)
    {
        sumOut += out[i];
    }
    double Ibias = sumOut / (((double)out.size() - (double)discard) * sipm.dt);
    double current_val;
    wstring current_prefix;
    tie(current_prefix, current_val) = exponent_val(Ibias);
    wcout << L"Simulated Ibias:\t" << current_val << current_prefix << L"A";

    return Ibias;
}

bool TEST_currents()
{
    string BAR_STRING(BARS, '=');
    cout << BAR_STRING << endl;
    cout << "BEGIN TEST: SiPM Nonlinearity and Bias Current Accuracy" << endl;
    cout << BAR_STRING << endl;
    const vector<double> irradiances = {0, 1e-4, 1e-3, 2e-3, 5e-3, 1e-2, 1e-1, 1e0};
    vector<vector<double>> expected_currents = {};
    vector<SiPM> DUTs = {};
    vector<string> sipm_names = {};
    vector<double> areas = {};

    double photonsPerDt;
    double current;
    const double bounds[2] = {0.75, 1.25};
    const double ePhoton = (speedOfLight * hPlanck) / 405E-9;
    SiPM sipm;

    // Test Cases for SiPM Bias Current

    sipm_names.push_back("J30020 2V Over");
    DUTs.push_back(SiPM(14410, 24.5 + 2, 24.5, 2.2 * 14e-9, 0.0, 4.6e-14, 2.04, 0.46, 0)); // J30020
    expected_currents.push_back({0, 4.5235e-5, 4.2184e-4, 8.2776e-4, 1.960e-3, 3.6916e-3, 18.0968e-3, 34.0636e-3});
    areas.push_back(pow((3.07E-3), 2));

    sipm_names.push_back("J30020 3V Over");
    DUTs.push_back(SiPM(14410, 24.5 + 3, 24.5, 2.2 * 14e-9, 0.0, 4.6e-14, 2.04, 0.46, 0)); // J30020
    expected_currents.push_back({0, 8.4519e-5, 8.0958e-4, 1.5820e-3, 3.7762e-3, 6.9938e-3, 31.4950e-3, 55.0066e-3});
    areas.push_back(pow((3.07E-3), 2));

    sipm_names.push_back("J30020 4V Over");
    DUTs.push_back(SiPM(14410, 24.5 + 4, 24.5, 2.2 * 14e-9, 0.0, 4.6e-14, 2.04, 0.46, 0)); // J30020
    expected_currents.push_back({0, 1.3120e-4, 1.2665e-3, 2.4768e-3, 5.8981e-3, 10.7652e-3, 45.7787e-3, 76.3823e-3});
    areas.push_back(pow((3.07E-3), 2));

    /*
    sipm_names.push_back("J60035 3V Over");
    DUTs.push_back(SiPM(14410, 27.5, 24.5, 2.2 * 14e-9, 0.0, 4.6e-14, 2.04, 0.46)); // J60035
    expected_currents.push_back({0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0});
    areas.push_back(pow((6E-3), 2));
    */

    bool passed = true;
    double testCurrent;

    wstring irrad_prefix, expected_current_prefix;
    double irrad_val, expected_current_val;

    for (int j = 0; j < (int)DUTs.size(); j++)
    {
        sipm = DUTs[j];
        sipm.dt = 1E-10;
        cout << "********** " << sipm_names[j] << " **********" << endl;
        for (int i = 0; i < (int)irradiances.size(); i++)
        {

            photonsPerDt = sipm.dt * irradiances[i] * areas[j] / ePhoton;

            tie(irrad_prefix, irrad_val) = exponent_val(irradiances[i]);
            tie(expected_current_prefix, expected_current_val) = exponent_val(expected_currents[j][i]);

            wcout << L"405nm Irradiance: " << irrad_val << irrad_prefix << L"W/m2   \tExpected Ibias: "
                  << expected_current_val << expected_current_prefix << L"A      \t";

            current = ibias_check(sipm, photonsPerDt);
            testCurrent = expected_currents[j][i];

            if ((current >= testCurrent * bounds[0]) & (current <= testCurrent * bounds[1]))
            {
                wcout << L"      \tPASS" << endl;
            }
            else
            {
                wcout << L"      \tFAIL" << endl;
                passed = false;
            }
        }
    }

    string outStatus = passed ? "PASS\n" : "FAIL\n";
    cout << BAR_STRING << endl;
    cout << "TEST " << outStatus;
    cout << "END TEST: SiPM Nonlinearity and Bias Current Accuracy" << endl;
    cout << BAR_STRING << endl;
    return passed;
}
