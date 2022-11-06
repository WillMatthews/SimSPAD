
#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <cmath>

#include "../src/sipm.hpp"
#include "../src/utilities.hpp"

using namespace std;

double speed_measure(double photonsPerDt)
{
    int speedTestSamples = 10000;
    vector<double> in(speedTestSamples, photonsPerDt); // DC light source
    SiPM sipm(14410, 27.5, 24.5, 2.2 * 14e-9, 0.0, 4.6e-14, 2.04, 0.46);
    sipm.dt = 1E-10;

    vector<double> out = {};

    bool silence = true;
    auto start = chrono::steady_clock::now();
    out = sipm.simulate(in, silence);
    auto end = chrono::steady_clock::now();

    (void) out; 
    chrono::duration<double> elapsed = end - start;

    double time_per_iter = ((double)elapsed.count()) / ((double)speedTestSamples);
    double time_per_ucell = time_per_iter / sipm.numMicrocell;

    return time_per_ucell;
}

bool TEST_performance()
{
    vector<double> photonsPerDt = {0, 1, 10, 100, 1000};
    vector<double> expected_runtimes = {250e-12, 250e-12, 250e-12, 325e-12, 400e-12};
    double runtime;
    bool passed = true;

    for (int i = 0; i < (int)photonsPerDt.size(); i++)
    {
        runtime = speed_measure(photonsPerDt[i]);
        cout << photonsPerDt[i] << "\t" << runtime << "\t";
        if (runtime < expected_runtimes[i])
        {
            cout << "OK" << endl;
        }
        else
        {
            cout << "FAIL" << endl;
            passed = false;
        }
    }
    return passed;
}

