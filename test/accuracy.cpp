
#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <cmath>

#include "../src/sipm.hpp"
#include "../src/utilities.hpp"

using namespace std;

double ibias_test(SiPM sipm, double photonsPerDt)
{
    int testSamples = 10000;
    vector<double> in(testSamples, photonsPerDt); // DC light source

    vector<double> out = {};
    SiPM sipm;

    auto start = chrono::steady_clock::now();
    bool silence = true;
    out = sipm.simulate(in, silence);

    double sumOut = 0;
    for (auto &a : outputVec){sumOut += a;}

    double Ibias = sumOut / (inputSize * dt);
    cout << "Simulated Ibias:\t" << Ibias * 1E3 << "mA" << endl;
    
    return Ibias;
}

template <class arrayType>
int arraySize(arrayType a)
{
    return sizeof(a) / sizeof(decltype(a[0]));
}

int main()
{
    vector<double> irradiances = {1e-4, 1e-3, 2e-3, 5e-3, 1e-2, 1e-1, 1e0};
    vector<vector<double>> expected_currents = {};
    vector<SiPM> DUTs = {};

    double current;
    double* bounds = {0.9, 1.1};

    DUTs[0] = SiPM(14410, 27.5, 24.5, 2.2 * 14e-9, 0.0, 4.6e-14, 2.04, 0.46);   // J30020
    expected_currnets[0] = {};
    DUTs[1] = SiPM(14410, 27.5, 24.5, 2.2 * 14e-9, 0.0, 4.6e-14, 2.04, 0.46);   // J60035
    expected_currnets[1] = {};

    bool passed = true;
    for (int j=0; j < (int)DUTs.size(); j++)
    {
        sipm = DUTs[j]; 
        sipm.dt = 1E-10;
        for (int i = 0; i < (int)irradiances.size(); i++)
        {
            cout << "====== " <<  sipm_names[j] <<  " ======"<< endl;
            cout << "405nm Irradiance: " << irradiances[i] << "W/m2\tExpected Ibias: " << expected_currents[j][i] << "A\t";
            photonsPerDt = 0; // TODO photon energy and area... convert irrad to photons per dt.
            current  = ibias_test(sipm, photonsPerDt)

            if (runtime < expected_runtimes[i])
            {
                cout << "\t\tOK" << endl;
            }
            else
            {
                cout << "\t\tFAIL" << endl;
                passed = false;
            }
        }
    }
    if (passed)
    {
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
