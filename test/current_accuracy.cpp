
#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <cmath>

#include "../src/sipm.hpp"
#include "../src/utilities.hpp"

using namespace std;

double ibias_check(SiPM sipm, double photonsPerDt)
{
    int testSamples = 10000;
    vector<double> in(testSamples, photonsPerDt); // DC light source

    vector<double> out = {};

    auto start = chrono::steady_clock::now();
    bool silence = true;
    out = sipm.simulate(in, silence);

    double sumOut = 0;
    for (auto &a : out){sumOut += a;}

    double Ibias = sumOut / (out.size() * sipm.dt);
    cout << "Simulated Ibias:\t" << Ibias * 1E3 << "mA";
    
    return Ibias;
}

bool TEST_currents()
{
    vector<double> irradiances = {1e-4, 1e-3, 2e-3, 5e-3, 1e-2, 1e-1, 1e0};
    vector<vector<double>> expected_currents = {};
    vector<SiPM> DUTs = {};
    vector<string> sipm_names = {};

    double photonsPerDt;
    double current;
    double bounds[2] = {0.9, 1.1};
    SiPM sipm;

    sipm_names.push_back("J30020");
    DUTs.push_back(SiPM(14410, 27.5, 24.5, 2.2 * 14e-9, 0.0, 4.6e-14, 2.04, 0.46));   // J30020
    expected_currents.push_back({1.0,1.0,1.0,1.0,1.0,1.0,1.0});

    sipm_names.push_back("J60035");
    DUTs.push_back(SiPM(14410, 27.5, 24.5, 2.2 * 14e-9, 0.0, 4.6e-14, 2.04, 0.46));   // J60035
    expected_currents.push_back({1.0,1.0,1.0,1.0,1.0,1.0,1.0});

    bool passed = true;
    double testCurrent;
    for (int j=0; j < (int)DUTs.size(); j++)
    {
        sipm = DUTs[j]; 
        sipm.dt = 1E-10;
        cout << "====== " <<  sipm_names[j] <<  " ======"<< endl;
        for (int i = 0; i < (int)irradiances.size(); i++)
        {
            cout << "405nm Irradiance: " << irradiances[i] << "W/m2\tExpected Ibias: " << expected_currents[j][i] << "A\t";
            photonsPerDt = 0; // TODO photon energy and area... convert irrad to photons per dt.
            current  = ibias_check(sipm, photonsPerDt);
            testCurrent = expected_currents[j][i];

            if ( (current > testCurrent * bounds[0] ) & (current < testCurrent * bounds[1]) )
            {
                cout << "\tOK" << endl;
            }
            else
            {
                cout << "\tFAIL" << endl;
                passed = false;
            }
        }
    }
    return passed;
}

