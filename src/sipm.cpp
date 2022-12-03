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
#include <algorithm>
#include <random>
#include "utilities.hpp"

// Progress bar defines
#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

using namespace std;

SiPM::SiPM(int numMicrocell_in, double vbias_in, double vBr_in, double tauRecovery_in,
           double tFwhm_in, double digitalThreshold_in, double cCell_in, double vChr_in, double pdeMax_in)
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
    pdeMax = pdeMax_in;                     // pdeMax characteristic for PDE-vOver curve

    microcellTimes = vector<double>(numMicrocell, 0.0); // microcell live time since last detection vector

    LUTSize = 20;
    tVecLUT = new double[LUTSize];
    pdeVecLUT = new double[LUTSize];
    vVecLUT = new double[LUTSize];

    seed_engines();
    precalculate_LUT();
    input_sanitation();
}

SiPM::SiPM(int numMicrocell_in, double vbias_in, double vBr_in, double tauRecovery_in,
           double digitalThreshold_in, double cCell_in, double vChr_in, double pdeMax_in)
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
    pdeMax = pdeMax_in;                     // pdeMax characteristic for PDE-vOver curve

    microcellTimes = vector<double>(numMicrocell, 0.0); // microcell live time since last detection vector

    LUTSize = 20;
    tVecLUT = new double[LUTSize];
    pdeVecLUT = new double[LUTSize];
    vVecLUT = new double[LUTSize];

    seed_engines();
    precalculate_LUT();
    input_sanitation();
}

SiPM::SiPM(vector<double> svars)
{
    dt = svars[0];
    numMicrocell = (int)svars[1]; // number of microcells in SiPM
    vBias = svars[2];             // supplied SiPM bias voltage
    vBr = svars[3];               // SiPM breakdown voltage
    tauRecovery = svars[4];       // recharge recovery time tau RC
    pdeMax = svars[5];            // pdeMax characteristic for PDE-vOver curve
    vChr = svars[6];              // characteristic voltage for PDE-vOver curve
    cCell = svars[7];             // microcell capacitance
    tauFwhm = svars[8];           // full width half max output pulse time
    digitalThreshold = svars[9];  // readout threshold (typically 0 for analog)
    vOver = vBias - vBr;          // overvoltage

    microcellTimes = vector<double>(numMicrocell, 0.0); // microcell live time since last detection vector

    LUTSize = 20;
    tVecLUT = new double[LUTSize];
    pdeVecLUT = new double[LUTSize];
    vVecLUT = new double[LUTSize];

    seed_engines();
    precalculate_LUT();
    input_sanitation();
}

SiPM::SiPM() {}

SiPM::~SiPM() {}

// convert overvoltage to PDE
inline double SiPM::pde_from_volt(double overvoltage)
{
    return pdeMax * (1 - exp(-(overvoltage / vChr)));
}

// convert time since last detection to PDE
inline double SiPM::pde_from_time(double time)
{
    double v = volt_from_time(time);
    return pde_from_volt(v);
}

// convert time since last detection to microcell voltage
inline double SiPM::volt_from_time(double time)
{
    return vOver * (1 - exp(-time / tauRecovery));
}

// Simulation function - takes as an argument a 'light' vector
// light vector is the expected number of photons to strike the SiPM in simulation time step dt.
vector<double> SiPM::simulate(vector<double> light, bool silent)
{
    vector<double> qFired = {};
    double percentDone;
    double l;
    double T = 0;
    init_spads(light);
    // O(light.size()* numMicrocell)
    for (int i = 0; i < (int)light.size(); i++)
    {
        if ((!silent) & (i % 10000 == 0 || i == (int)(light.size() - 1)))
        {
            percentDone = (double)i / (double)(light.size() - 1);
            print_progress(percentDone);
        }
        l = light[i];
        if (l < 0)
        {
            l = 0;
        }
        qFired.push_back(simulate_microcells(T, l));
        T += dt;
    }
    return qFired;
}

vector<double> SiPM::shape_output(vector<double> inputVec)
{
    vector<double> kernel = get_gaussian(dt, tauFwhm);

    return conv1d(inputVec, kernel);
}

// randomly seed engines
void SiPM::seed_engines()
{
    poissonEngine.seed(random_device{}());
    unifRandomEngine.seed(random_device{}());
    renewalEngine.seed(random_device{}());
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
void SiPM::init_spads(vector<double> light) // inclusion adds ~ 35ps/ucell dt in SIM
{
    double meanInPhotonsDt = 0;
    for (auto &a : light)
    {
        meanInPhotonsDt += a;
    }
    meanInPhotonsDt = meanInPhotonsDt / light.size();
    if (meanInPhotonsDt == 0)
    {
        // prevent errors with distribution generation - assume one photon arriving?
        meanInPhotonsDt = 1 / (double)light.size();
    }

    // Define and generate Inter-Detection distribution

    // Generate rate parameter for arriving photons
    double lambda = meanInPhotonsDt / (dt * numMicrocell);
    double tmax = tauRecovery * 20; // how to I estimate a good tmax?
    int nIntegralPDE = 200;         // how many elements are needed? TODO remove this magic number
    int nPDF = 1500;                // how many elements are needed? TODO remove this magic number

    vector<double> f_t;
    vector<double> T;
    for (int i = 0; i < nPDF; i++)
    {
        double t = i * tmax / (double)nPDF;
        // p_t(t) provides the approximation of \frac{1}{t} \int_0^t pde(t) dt
        double p_t = t > 0 ? trapezoidal(&SiPM::pde_from_time, 0.0, t, nIntegralPDE) / t : 0;

        f_t.push_back(pde_from_time(t) * lambda * exp(-lambda * t * p_t));
        T.push_back(t);
    }

    // do integration: \int_t^{\infty} f_t(t) dt
    // As weights are produced for the distribution - we do not need to normalise
    reverse(f_t.begin(), f_t.end());
    vector<double> weights = cum_trapezoidal(f_t, T[1] - T[0]);
    reverse(weights.begin(), weights.end());

    // Generate time since last detection distribution f_x = \frac {\int_t^{\infty} f_t(t) dt} {\int_0^{\infty} \int_t^{\infty} f_t(t) dt dt}
    // use piecewise linear as an approximation
    std::piecewise_constant_distribution<>
        d(T.begin(), T.end(), weights.begin());

    // randomly sample this distribution
    for (int i = 0; (int)i < numMicrocell; i++)
    {
        microcellTimes[i] = -d(renewalEngine); // negative as in the past - before simulation has begun
    }
}

double SiPM::simulate_microcells(double T, double photonsPerDt)
{
    double output = 0;
    double volt = 0;

    // randomly sample poisson parameter lambda input to generate number of incoming photons
    poisson_distribution<int> distribution(photonsPerDt);
    int poissonPhotons = distribution(poissonEngine);

    // generate n random microcells to strike - n random microcells
    vector<int> struckMicrocells = {}; // DO NOT REPLACE WITH SET! set is too slow.
    for (int i = 0; (int)i < poissonPhotons; i++)
    {
        struckMicrocells.push_back(unif_rand_int(0, numMicrocell));
    }

    for (auto &i : struckMicrocells)
    {
        if (T == microcellTimes[i]) // if ucell has already been struck, skip it
        {
            continue;
        }
        if (unif_rand_double(0, 1) < (pde_LUT(T - microcellTimes[i]))) // PDE detection test
        {
            volt = volt_LUT(T - microcellTimes[i]); // calculate ucell voltage
            microcellTimes[i] = T;                  // set detection time
            if (volt > digitalThreshold * vOver)
            {
                output += volt * cCell; // add to output
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

// Input Sanitation Checks - currently just throws exceptions!
void SiPM::input_sanitation() // inclusion adds ~ 10ps/ucell dt in SIM
{
    if (dt <= 0)
    {
        dt = 1E-100;
        invalid_argument("dt cannot be less than or equal to zero");
    }
    if (numMicrocell <= 0)
    {
        numMicrocell = 1;
        invalid_argument("numMicrocell cannot be less than or equal to zero");
    }
    if (vOver < 0)
    {
        vOver = 0;
        invalid_argument("Overvoltage cannot be less than zero");
    }
    if (tauRecovery < 0)
    {
        tauRecovery = 0;
        invalid_argument("Recovery time constant cannot be less than zero");
    }
    if (pdeMax < 0)
    {
        pdeMax = 0;
        invalid_argument("PDEmax constant cannot be less than zero");
    }
    if (pdeMax > 1)
    {
        pdeMax = 1;
        invalid_argument("PDEmax constant cannot be greater than one");
    }
    if (vChr <= 0)
    {
        vChr = 1;
        invalid_argument("PDE characteristic voltage cannot be less than or equal to zero");
    }
    /* if (cCell < 0) // this is fine  - just inverts output
    {
        invalid_argument("Microcell capacitance cannot be less than zero");
    }
    */
    /* if (tauFwhm <= 0) // not yet implemented
    {
        invalid_argument("Output pulse width cannot be less than or equal to zero");
    }*/
    if (digitalThreshold < 0)
    {
        digitalThreshold = 0;
        invalid_argument("Digital Threshold cannot be less than zero");
    }
}

//// LOOKUP TABLE PARAMS AND FUNCTIONS

void SiPM::precalculate_LUT(void)
{
    const int numPoints = (int)LUTSize;
    const double maxTime = tauRecovery > 0 ? 5.3 * tauRecovery : 1E-9;
    const double ddt = (double)maxTime / numPoints;
    for (int i = 0; i < numPoints; i++)
    {
        tVecLUT[i] = i * ddt;
        vVecLUT[i] = vOver * (1 - exp(-tVecLUT[i] / tauRecovery));
        pdeVecLUT[i] = pde_from_volt(vVecLUT[i]);
    }
}

// photon detection efficiency as a function of time lookup table
double SiPM::pde_LUT(double x) const
{
    return LUT(x, pdeVecLUT);
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

    // this check first - Microcell more likely to be recharged under low arrival rate scenarios
    if (x > xs[count - 1])
    {
        return ys[count - 1]; // return maximum
    }
    /*
    // Impossible to enter this region of LUT - time vector starts at zero
    // no negative time in simulation!
    if (x < xs[0])
    {
        // x is less than the minimum element
        //  handle error here if you want
        return ys[0]; // return minimum element
    }
    */
    // find i, such that xs[i] <= x < xs[i+1]
    for (i = 0; i < count - 1; i++)
    {
        if (xs[i + 1] > x)
        {
            break;
        }
    }
    // interpolate in the region of the LUT where xs[i] <= x < xs[i+1]
    dx = xs[i + 1] - xs[i];
    dy = ys[i + 1] - ys[i];
    return ys[i] + (x - xs[i]) * dy / dx;
}

double SiPM::trapezoidal(double (SiPM::*f)(double), double lower, double upper, int n)
{
    double dx = (upper - lower) / n;
    double s = (this->*f)(lower) + (this->*f)(upper); // beginning and end add to formula

    for (int i = 1; i < (n - 1); i++)
    {
        s += 2 * (this->*f)(lower + i * dx);
    }
    return dx * s / 2;
}