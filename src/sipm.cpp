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
#include <cmath>
#include <random>
#include "utilities.hpp"

// Progress bar defines
#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

using namespace std;

SiPM::SiPM(int numMicrocell_in, double vbias_in, double vBr_in, double tauRecovery_in,
           double tFwhm_in, double digitalThreshold_in, double cCell_in, double vChr_in, double pdeMax1_in, double pdeMax2_in)
{
    numMicrocell = numMicrocell_in;         // number of microcells in SiPM
    vBias = vbias_in;                       // supplied SiPM bias voltage
    vBr = vBr_in;                           // SiPM breakdown voltage
    tauRecovery = tauRecovery_in;           // recharge recovery time tau RC
    tauFwhm = tFwhm_in;                     // full width half max output pulse time
    digitalThreshold = digitalThreshold_in; // readout threshold (typically 0 for analog)
    cCell = cCell_in;                       // microcell capacitance
    vOver = vbias_in - vBr_in;              // overvoltage
    vChr = vChr_in;                         // characteristic voltage for PDE-vOver curve
    pdeMax1 = pdeMax1_in;                     // pdeMax characteristic for PDE-vOver curve
    pdeMax2 = pdeMax2_in;                     // pdeMax characteristic for PDE-vOver curve

    microcellTimes = vector<double>(numMicrocell, 0.0); // microcell live time since last detection vector

    LUTSize = 20;
    tVecLUT = new double[LUTSize];
    pde1VecLUT = new double[LUTSize];
    pde2VecLUT = new double[LUTSize];
    vVecLUT = new double[LUTSize];

    seed_engines();
    precalculate_LUT();
}

SiPM::SiPM(int numMicrocell_in, double vbias_in, double vBr_in, double tauRecovery_in,
           double digitalThreshold_in, double cCell_in, double vChr_in, double pdeMax1_in, double pdeMax2_in)
{
    numMicrocell = numMicrocell_in;         // number of microcells in SiPM
    vBias = vbias_in;                       // supplied SiPM bias voltage
    vBr = vBr_in;                           // SiPM breakdown voltage
    tauRecovery = tauRecovery_in;           // recharge recovery time tau RC
    tauFwhm = 0;                            // full width half max output pulse time
    digitalThreshold = digitalThreshold_in; // readout threshold (typically 0 for analog)
    cCell = cCell_in;                       // microcell capacitance
    vOver = vbias_in - vBr_in;              // overvoltage
    vChr = vChr_in;                         // characteristic voltage for PDE-vOver curve
    pdeMax1 = pdeMax1_in;                   // pdeMax characteristic for PDE-vOver curve
    pdeMax2 = pdeMax2_in;                   // pdeMax characteristic for PDE-vOver curve

    microcellTimes = vector<double>(numMicrocell, 0.0); // microcell live time since last detection vector

    LUTSize = 20;
    tVecLUT = new double[LUTSize];
    pde1VecLUT = new double[LUTSize];
    pde2VecLUT = new double[LUTSize];
    vVecLUT = new double[LUTSize];

    seed_engines();
    precalculate_LUT();
}

SiPM::SiPM(vector<double> svars)
{
    dt = svars[0];
    numMicrocell = (int)svars[1]; // number of microcells in SiPM
    vBias = svars[2];             // supplied SiPM bias voltage
    vBr = svars[3];               // SiPM breakdown voltage
    tauRecovery = svars[4];       // recharge recovery time tau RC
    pdeMax1 = svars[5];           // pdeMax characteristic for PDE-vOver curve wavelength 1
    pdeMax2 = svars[6];           // pdeMax characteristic for PDE-vOver curve wavelength 2
    vChr = svars[7];              // characteristic voltage for PDE-vOver curve
    cCell = svars[8];             // microcell capacitance
    tauFwhm = svars[9];           // full width half max output pulse time
    digitalThreshold = svars[10]; // readout threshold (typically 0 for analog)
    vOver = vBias - vBr;          // overvoltage

    microcellTimes = vector<double>(numMicrocell, 0.0); // microcell live time since last detection vector

    LUTSize = 20;
    tVecLUT = new double[LUTSize];
    pde1VecLUT = new double[LUTSize];
    pde2VecLUT = new double[LUTSize];
    vVecLUT = new double[LUTSize];

    seed_engines();
    precalculate_LUT();
}

SiPM::SiPM() {}

SiPM::~SiPM() {}

// convert overvoltage to PDE
inline double SiPM::pde1_from_volt(double overvoltage)
{
    return pdeMax1 * (1 - exp(-(overvoltage / vChr)));
}

// convert time since last detection to PDE
inline double SiPM::pde1_from_time(double time)
{
    double v = volt_from_time(time);
    return pde1_from_volt(v);
}

inline double SiPM::pde2_from_volt(double overvoltage)
{
    return pdeMax2 * (1 - exp(-(overvoltage / vChr)));
}

// convert time since last detection to PDE
inline double SiPM::pde2_from_time(double time)
{
    double v = volt_from_time(time);
    return pde2_from_volt(v);
}

// convert time since last detection to microcell voltage
inline double SiPM::volt_from_time(double time)
{
    return vOver * (1 - exp(-time / tauRecovery));
}

// Simulation function - takes as an argument a 'light' vector
// light vector is the expected number of photons to strike the SiPM in simulation time step dt.
vector<double> SiPM::simulate(vector<double> light1, vector<double> light2, bool silent)
{
    vector<double> qFired = {};
    double percentDone;
    double l1, l2;
    init_spads(light1, light2);
    // O(light.size()* numMicrocell)
    for (int i = 0; i < (int)light1.size(); i++)
    {
        if ((!silent) & (i % 10000 == 0 || i == (int)(light1.size() - 1)))
        {
            percentDone = (double)i / (double)(light1.size() - 1);
            print_progress(percentDone);
        }
        l1 = light1[i];
        l2 = light2[i];
        if (l1 < 0)
        {
            l1 = 0;
        }
        if (l2 < 0)
        {
            l2 = 0;
        }
        qFired.push_back(selective_recharge_illuminate_LUT(l1, l2));
    }
    return qFired;
}

// randomly seed engines
void SiPM::seed_engines()
{
    poissonEngine.seed(random_device{}());
    unifRandomEngine.seed(random_device{}());
    exponentialEngine.seed(random_device{}());
}

// random double between range a and b
double SiPM::unif_rand_double(double a, double b)
{
    return unif(unifRandomEngine) * (b - a) + a;
}

// Uniform random integer. Do not change - this is fast
int SiPM::unif_rand_int(int a, int b)
{
    return (int)(a + static_cast<double>(rand()) / (static_cast<double>(RAND_MAX / (b - a))));
}

//// SIMULATION METHODS

// Initialises SiPMs based on an exponential distribution.
// This is definitely wrong - The distribution is more complicated, but this is better than uniform.
void SiPM::init_spads(vector<double> light1, vector<double> light2)
{
    double meanInPhotonsDt1 = 0;
    double meanInPhotonsDt2 = 0;
    for (auto &a : light1)
    {
        meanInPhotonsDt1 += a;
    }
    for (auto &a : light2)
    {
        meanInPhotonsDt2 += a;
    }
    meanInPhotonsDt1 = meanInPhotonsDt1 / light1.size();
    meanInPhotonsDt2 = meanInPhotonsDt1 / light2.size();
    if (meanInPhotonsDt1 == 0)
    {
        meanInPhotonsDt1 = 1E-10; // prevent errors with the exponential distribution generation
    }
    if (meanInPhotonsDt2 == 0)
    {
        meanInPhotonsDt2 = 1E-10; // prevent errors with the exponential distribution generation
    }

    // predict PDE - assume each microcell recovers to Vbias
    double estPde1 = pdeMax1 * (1 - exp(-(vBias - vBr) / vChr));
    double estPde2 = pdeMax1 * (1 - exp(-(vBias - vBr) / vChr));

    // Generate Distribution and rate parameter
    double lambda = (estPde1 * meanInPhotonsDt1 + estPde2 * meanInPhotonsDt2) / (dt * numMicrocell);
    exponential_distribution<double> expDistribution(lambda);

    // randomly sample
    for (int i = 0; (int)i < numMicrocell; i++)
    {
        microcellTimes[i] = expDistribution(exponentialEngine);
    }
}

double SiPM::selective_recharge_illuminate_LUT(double photonsPerDt1, double photonsPerDt2)
{
    double output = 0;
    double volt = 0;

    // randomly sample poisson parameter lambda input to generate number of incoming photons
    poisson_distribution<int> distribution1(photonsPerDt1);
    int poissonPhotons1 = distribution1(poissonEngine);
    poisson_distribution<int> distribution2(photonsPerDt2);
    int poissonPhotons2 = distribution2(poissonEngine);

    // generate n random microcells to strike - n random microcells
    vector<int> struckMicrocells1 = {}; // DO NOT REPLACE WITH SET! set is too slow.
    vector<int> struckMicrocells2 = {};

    for (int i = 0; (int)i < poissonPhotons1; i++)
    {
        struckMicrocells1.push_back(unif_rand_int(0, numMicrocell));
    }
    for (int i = 0; (int)i < poissonPhotons2; i++)
    {
        struckMicrocells2.push_back(unif_rand_int(0, numMicrocell));
    }

    // recharge all microcells
    for (int i = 0; i < numMicrocell; i++)
    {
        microcellTimes[i] += dt;
    }

    for (auto &i : struckMicrocells1)
    {
        if (unif_rand_double(0, 1) < (pde1_LUT(microcellTimes[i])))
        {
            volt = volt_LUT(microcellTimes[i]);
            microcellTimes[i] = 0;
            if (volt > digitalThreshold * vOver)
            {
                output += volt * cCell;
            }
        }
    }
    for (auto &i : struckMicrocells2)
    {
        if (unif_rand_double(0, 1) < (pde2_LUT(microcellTimes[i])))
        {
            volt = volt_LUT(microcellTimes[i]);
            microcellTimes[i] = 0;
            if (volt > digitalThreshold * vOver)
            {
                output += volt * cCell;
            }
        }
    }
    return output;
}

//// UTILITY FUNCTIONS

// progress bar
void SiPM::print_progress(double percentage) const
{
    int val = (int)(percentage * 100);
    int lpad = (int)(percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush(stdout);
    if (val == 100)
    {
        cout << "\n"
             << endl;
    }
}

// Sanity check random number generation as used in the program
void SiPM::test_rand_funcs()
{
    const int numStars = 100;    // maximum number of stars to distribute
    const int numIntervals = 10; // number of intervals
    int iters = 10000;

    cout << "\n\n***** UNIFORM *****\n"
         << endl;
    double s1;
    double test1 = 0;
    int p1[numIntervals] = {};
    for (int i = 0; i < iters; i++)
    {
        s1 = unif_rand_double(0, 1);
        test1 += s1;
        ++p1[int(numIntervals * s1)];
    }
    cout << "mean dist 1: " << test1 / iters << endl;

    for (int i = 0; i < numIntervals; i++)
    {
        cout << float(i) / numIntervals << "-" << float(i + 1) / numIntervals << "\t: ";
        cout << string(p1[i] * numStars / iters, '*') << endl;
    }

    double lambda = 3.5;
    const int poissonNumIntervals = 20;
    int p2[poissonNumIntervals] = {};
    int s2;
    for (int i = 0; i < iters; i++)
    {
        poisson_distribution<int> distribution(lambda);
        s2 = distribution(poissonEngine);
        ++p2[int(s2)];
    }

    cout << "\n\n***** POISSON *****\n"
         << endl;
    for (int i = 0; i < poissonNumIntervals; i++)
    {
        cout << i << "\t: ";
        cout << string(p2[i] * numStars / iters, '*') << endl;
    }
}

vector<double> SiPM::shape_output(vector<double> inputVec)
{
    vector<double> kernel = get_gaussian(dt, tauFwhm);

    return conv1d(inputVec, kernel);
}

//// LOOKUP TABLE PARAMS AND FUNCTIONS

void SiPM::precalculate_LUT(void)
{
    const int numPoints = (int)LUTSize;
    const double maxTime = 5.3 * tauRecovery;
    const double ddt = (double)maxTime / numPoints;
    for (int i = 0; i < numPoints; i++)
    {
        tVecLUT[i] = i * ddt;
        vVecLUT[i] = vOver * (1 - exp(-tVecLUT[i] / tauRecovery));
        pde1VecLUT[i] = pde1_from_volt(vVecLUT[i]);
        pde2VecLUT[i] = pde2_from_volt(vVecLUT[i]);
    }
}

// photon detection efficiency as a function of time lookup table
double SiPM::pde1_LUT(double x) const
{
    return LUT(x, pde1VecLUT);
}

double SiPM::pde2_LUT(double x) const
{
    return LUT(x, pde2VecLUT);
}

// ucell voltage as a function of time lookup table
double SiPM::volt_LUT(double x) const
{
    return LUT(x, vVecLUT);
}

// define a generic lookup table that works on with the time vector
double SiPM::LUT(double x, double *workingVector) const
{
    double *xs = tVecLUT;
    double *ys = workingVector;
    // number of elements in the array
    const int count = LUTSize;
    int i;
    double dx, dy;

    if (x < xs[0])
    {
        // x is less than the minimum element
        //  handle error here if you want
        return ys[0]; // return minimum element
    }
    if (x > xs[count - 1])
    {
        return ys[count - 1]; // return maximum
    }
    // find i, such that xs[i] <= x < xs[i+1]
    for (i = 0; i < count - 1; i++)
    {
        if (xs[i + 1] > x)
        {
            break;
        }
    }
    // o.t.w. interpolate
    dx = xs[i + 1] - xs[i];
    dy = ys[i + 1] - ys[i];
    return ys[i] + (x - xs[i]) * dy / dx;
}
