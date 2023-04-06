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

void cli_logo(void)
{
    cout << "   _____ _          _____ ____  ___    ____ " << endl;
    cout << "  / ___/(_)___ ___ / ___// __ \\/   |  / __ \\" << endl;
    cout << "  \\__ \\/ / __ `__  \\__ \\/ /_/ / /| | / / / /" << endl;
    cout << " ___/ / / / / / / /__/ / ____/ ___ |/ /_/ / " << endl;
    cout << "/____/_/_/ /_/ /_/____/_/   /_/  |_/_____/  \n"
         << endl;
}

SiPM::SiPM(unsigned long numMicrocell_in, double vbias_in, double vBr_in, double tauRecovery_in,
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

    microcellTimes = vector<double>{};        // microcell live time of last detection vector
    microcellTimes.reserve(numMicrocell + 8); // Prevent issues with free()

    LUTSize = 20;                    // Look Up Table Size
    tVecLUT = new double[LUTSize];   // Preallocate LUT Time array
    pdeVecLUT = new double[LUTSize]; // Preallocate LUT PDE array
    vVecLUT = new double[LUTSize];   // Preallocate LUT Voltage array

    seed_engines();
    precalculate_LUT();
    input_sanitation();
}

SiPM::SiPM(unsigned long numMicrocell_in, double vbias_in, double vBr_in, double tauRecovery_in,
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

    microcellTimes = vector<double>{};        // microcell live time of last detection vector
    microcellTimes.reserve(numMicrocell + 8); // Prevent issues with free()

    LUTSize = 20;                    // Look Up Table Size
    tVecLUT = new double[LUTSize];   // Preallocate LUT Time array
    pdeVecLUT = new double[LUTSize]; // Preallocate LUT PDE array
    vVecLUT = new double[LUTSize];   // Preallocate LUT Voltage array

    seed_engines();
    precalculate_LUT();
    input_sanitation();
}

SiPM::SiPM(vector<double> svars)
{
    dt = svars[0];
    numMicrocell = (unsigned long)svars[1]; // number of microcells in SiPM
    vBias = svars[2];                       // supplied SiPM bias voltage
    vBr = svars[3];                         // SiPM breakdown voltage
    tauRecovery = svars[4];                 // recharge recovery time tau RC
    pdeMax = svars[5];                      // pdeMax characteristic for PDE-vOver curve
    vChr = svars[6];                        // characteristic voltage for PDE-vOver curve
    cCell = svars[7];                       // microcell capacitance
    tauFwhm = svars[8];                     // full width half max output pulse time
    digitalThreshold = svars[9];            // readout threshold (typically 0 for analog)
    vOver = vBias - vBr;                    // overvoltage

    microcellTimes = vector<double>{};        // microcell live time of last detection vector
    microcellTimes.reserve(numMicrocell + 8); // Prevent issues with free()

    LUTSize = 20;                    // Look Up Table Size
    tVecLUT = new double[LUTSize];   // Preallocate LUT Time array
    pdeVecLUT = new double[LUTSize]; // Preallocate LUT PDE array
    vVecLUT = new double[LUTSize];   // Preallocate LUT Voltage array

    seed_engines();
    precalculate_LUT();
    input_sanitation();
}

SiPM::SiPM() {}

SiPM::~SiPM()
{
    cout << "Freed SiPM" << endl;
} // destructor

// Convert overvoltage to PDE
inline double SiPM::pde_from_volt(double overvoltage)
{
    return pdeMax * (1 - exp(-(overvoltage / vChr)));
}

// Convert time since last detection to PDE
inline double SiPM::pde_from_time(double time)
{
    double v = volt_from_time(time);
    return pde_from_volt(v);
}

// Convert time since last detection to microcell voltage
inline double SiPM::volt_from_time(double time)
{
    return vOver * (1 - exp(-time / tauRecovery));
}

// Simulation function - takes as an argument a 'light' vector
// light vector is the expected number of photons to strike the SiPM in simulation time step dt.
vector<double> SiPM::simulate(vector<double> light, bool silent)
{
    vector<double> qFired = {};
    qFired.reserve(light.size());
    double percentDone;
    double l;
    double T = 0;

    // Separate out SiPM initialisation? Being integrated it messes with timing.
    init_spads(light);

    // O(light.size()* numMicrocell)
    for (unsigned long i = 0; i < (unsigned long)light.size(); i++)
    {
        if ((!silent) & (i % 10000 == 0 || i == (unsigned long)(light.size() - 1)))
        {
            percentDone = (double)i / (double)(light.size() - 1);
            print_progress(percentDone);
        }

        // If expected num of photons per bit is negative, set to zero
        l = light[i] > 0.0 ? light[i] : 0.0;
        qFired.push_back(simulate_microcells(T, l));
        T += dt;
    }
    return qFired;
}

// Shapes the output of the SiPM with a Gaussian pulse
// Convolves a Gaussian with the output.
vector<double> SiPM::shape_output(vector<double> inputVec)
{
    vector<double> kernel = get_gaussian(dt, tauFwhm);
    return conv1d(inputVec, kernel);
}

// Seed Random Engines
// TODO improve this code - appears to give the same result for all runs within the same second
void SiPM::seed_engines()
{
    poissonEngine.seed(random_device{}());
    unifRandomEngine.seed(random_device{}());
    renewalEngine.seed(random_device{}());
}

// Random double between range a and b.
double SiPM::unif_rand_double(double a, double b)
{
    return unif(unifRandomEngine) * (b - a) + a;
}

// Uniform random integer. Do not change - this is fast
unsigned long SiPM::unif_rand_int(unsigned long a, unsigned long b)
{
    return (unsigned long)(a + static_cast<double>(rand()) / (static_cast<double>(RAND_MAX / (b - a))));
}

//// SIMULATION METHODS

// Initialises SiPMs based on an exponential distribution.
// This is correct for constant photon arrival rates.
// The true distribution for general input is more complicated and needs investigation.
// This is run at simulation time.
void SiPM::init_spads(vector<double> light) // inclusion adds ~ 35ps/ucell dt in SIM
{
    double meanInPhotonsDt = 0; // mean number of photons per time step
    if (!light.empty())
    {
        meanInPhotonsDt = reduce(light.begin(), light.end()) / (double)light.size();
    }
    if (meanInPhotonsDt == 0)
    {
        // prevent errors with distribution generation - assume one photon arriving?
        meanInPhotonsDt = 1 / (double)light.size();
    }

    // Define and generate Inter-Detection distribution
    // Generate rate parameter for arriving photons
    double lambda = meanInPhotonsDt / (dt * numMicrocell);
    double tmax = tauRecovery * 20;          // how to I estimate a good tmax?
    unsigned long upsampleIntegralPDE = 100; // how many elements are needed? TODO remove this magic number
    unsigned long nPDF = 1500;               // how many elements are needed? TODO remove this magic number
    double t;                                // Time

    // Inter detection time PDF
    // f_t(t) = $pde(t) \lambda exp(-\lambda * t * p_t(t))$
    vector<double> f_t = vector<double>(nPDF, 0.0);
    vector<double> T = vector<double>(nPDF, 0.0); // Time vector 1xnPDF

    for (unsigned long i = 0; i < nPDF; i++)
    {
        t = i * tmax / (double)nPDF;
        T[i] = t; // populate time vector
    }

    // p_t is the integral of the PDE from 0 to t divided by t
    // p_t(t) provides the approximation of $\frac{1}{t} \int_0^t pde(t) dt$
    // this line calculates the integral, the division is carried out in the loop on the elements that are needed
    vector<double> p_t = cum_trapezoidal(&SiPM::pde_from_time, 0.0, t, nPDF * upsampleIntegralPDE);

    for (unsigned long i = 0; i < nPDF; i++)
    {
        t = T[i];
        p_t[i * upsampleIntegralPDE] = t > 0.0 ? p_t[i * upsampleIntegralPDE] / t : 0.0;
        // calculate the interdetection probability at time t
        // Do not replace with LUT - solutions appear unstable
        f_t[i] = pde_from_time(t) * lambda * exp(-lambda * t * p_t[i * upsampleIntegralPDE]);
    }

    // do integration: \int_t^{\infty} f_t(t) dt
    // As weights are produced for the distribution - we do not need to normalise
    reverse(f_t.begin(), f_t.end());

    // cout << f_t[999] << endl; // TODO CALCULATE TMAX FROM LAMBDA!

    vector<double> weights = cum_trapezoidal(f_t, T[1] - T[0]); // PDF of time since detection for random stopping time
    reverse(weights.begin(), weights.end());

    // Generate time since last detection distribution
    // $f_x(t) = \frac {\int_t^{\infty} f_t(t) dt} {\int_0^{\infty} \int_t^{\infty} f_t(t) dt dt}$
    // use piecewise linear as an approximation
    std::piecewise_constant_distribution<> d(T.begin(), T.end(), weights.begin());

    // randomly sample this distribution
    for (unsigned long i = 0; (unsigned long)i < numMicrocell; i++)
    {
        microcellTimes.push_back(-d(renewalEngine)); // negative as in the past - before simulation has begun
    }
}

// For a single time step, simulate all the microcells in the SiPM detector
// This function relies on the internal private state microcellTimes, which stores the
// times when the last detection occured for each microcell.
// Inputs are the current time T, and the expected number of photons for this time step
// arriving at the detector
double SiPM::simulate_microcells(double T, double photonsPerDt)
{
    double output = 0; // output charge for a single time step
    double volt = 0;   // voltage for microcell

    // randomly sample poisson parameter lambda input to generate number of incoming photons
    poisson_distribution<int> distribution(photonsPerDt);
    unsigned long poissonPhotons = distribution(poissonEngine); // Number of incident photons

    unsigned long struck_cell;                         // index of the microcell struck with a photon
    for (unsigned long j = 0; j < poissonPhotons; j++) // for each incident photon...
    {
        // randomly strike a microcell (obtain the index)
        struck_cell = unif_rand_int(0, numMicrocell);

        if (T == microcellTimes[struck_cell]) // if ucell has already been struck, skip it INVALID READ
        {
            continue;
        }
        if (unif_rand_double(0, 1) < pde_LUT(T - microcellTimes[struck_cell])) // PDE detection test INVALID READ
        {
            volt = volt_LUT(T - microcellTimes[struck_cell]); // calculate ucell voltage INVALID READ
            microcellTimes[struck_cell] = T;                  // set detection time INVALID WRITE
            if (volt > digitalThreshold * vOver)              // digital threshold test
            {
                output += volt * cCell; // add fired microcell to output
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

/*
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
*/

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

// Initialise lookup table for the SiPM being simulated
// this is run at construction time
void SiPM::precalculate_LUT(void)
{
    // Number of points in lookup table (TODO kill this line?)
    const int numPoints = (int)LUTSize;
    // Determine the time range of the lookup table
    const double maxTime = tauRecovery > 0 ? 5.3 * tauRecovery : 1E-9;
    // dt for elements in the lookup table
    const double ddt = (double)maxTime / numPoints;
    for (int i = 0; i < numPoints; i++)
    {
        tVecLUT[i] = i * ddt;                                      // Time
        vVecLUT[i] = vOver * (1 - exp(-tVecLUT[i] / tauRecovery)); // Voltage
        pdeVecLUT[i] = pde_from_volt(vVecLUT[i]);                  // PDE
    }
}

// Photon detection efficiency as a function of time lookup table
double SiPM::pde_LUT(double x) const
{
    return LUT(x, pdeVecLUT);
}

// ucell voltage as a function of time lookup table
double SiPM::volt_LUT(double x) const
{
    return LUT(x, vVecLUT);
}

// Define a generic lookup table that works on with the time vector
double SiPM::LUT(double x, double *workingVector) const
{
    double *xs = tVecLUT;               // Precalculated LUT elements
    double *ys = workingVector;         // Precalculated LUT elements
    const unsigned int count = LUTSize; // Number of elements in the array
    unsigned int i;                     // Index to iterate over
    double dx, dy;                      // Differentials

    // Check if fully recharge first.
    // Microcell more likely to be recharged under low arrival rate scenarios
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
    // Find i, such that xs[i] <= x < xs[i+1]
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

// Trapezoidal integration of function f between the limits lower and upper, with n points
double SiPM::trapezoidal(double (SiPM::*f)(double), double lower, double upper, unsigned long n)
{
    // Step size of integral
    double dx = (upper - lower) / n;

    // Beginning and end add to formula
    double s = (this->*f)(lower) + (this->*f)(upper);

    for (unsigned long i = 1; i < (n - 1); i++)
    {
        s += 2 * (this->*f)(lower + i * dx);
    }
    return dx * s / 2;
}

// Cumunulative Trapezoidal integration of function f between the limits lower and upper, with n points
vector<double> SiPM::cum_trapezoidal(double (SiPM::*f)(double), double lower, double upper, unsigned long n)
{

    // create output vector
    vector<double> output = {};
    output.reserve(n);

    // Step size of integral
    double dx = (upper - lower) / n;

    // Beginning and end add to formula
    double s = (this->*f)(lower)*dx / 2;
    output.push_back(s);

    for (unsigned long i = 1; i < (n - 1); i++)
    {
        s += dx * (this->*f)(lower + i * dx);
        output.push_back(s);
    }

    s += (this->*f)(upper)*dx / 2;
    output.push_back(s);

    return output;
}

// Trapezoidally integrate vector f with spacing dx
double SiPM::trapezoidal(vector<double> f, double dx)
{
    double s = f[0] + f[f.size() - 1]; // Beginning and end add to formula
    for (int i = 1; i < (int)(f.size() - 1); i++)
    {
        s += 2 * f[i]; // Trapezoidal Integration
    }

    return dx * s / 2;
}

// Cumunulatively trapezoidally integrate vector f with spacing dx
vector<double> SiPM::cum_trapezoidal(vector<double> f, double dx)
{
    vector<double> result;
    result.reserve(f.size());

    double s = f[0] * dx / 2; // Beginning and end add to formula
    result.push_back(s);

    for (int i = 1; i < (int)(f.size() - 1); i++)
    {
        s += 2 * f[i] * dx / 2;
        result.push_back(s);
    }
    s += f[f.size() - 1] * dx / 2; // End add to formula
    result.push_back(s);

    return result;
}
