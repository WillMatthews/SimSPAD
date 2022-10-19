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
//#include <thread>
//#include <mutex>

// Progress bar defines
#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

using namespace std;

class SiPM
{

public:
    int numMicrocell;         // Number of Microcells
    double vbias;             // Bias Voltage
    double vbr;               // Breakdown Voltage
    double vover;             // Overvoltage (internally calculated - make private)
    double tauRecovery;       // Microcell Recharge RC Time Constant
    double digitalThreshhold; // Output Digital Threshhold for readout (0 for analog)
    double ccell;             // Microcell Capacitance
    double dt;                // Simulation Timestep
    double Vchr;              // Characteristic Voltage for PDE vs Vover curve
    double PDE_max;           // PDE_max parameter for PDE vs Vover curve

    // constructor
    SiPM(int numMicrocell_in, double vbias_in, double vbr_in, double tauRecovery_in, double digitalThreshhold_in, double ccell_in, double Vchr_in, double PDE_max_in)
    {
        numMicrocell = numMicrocell_in;
        vbias = vbias_in;
        vbr = vbr_in;
        tauRecovery = tauRecovery_in;
        digitalThreshhold = digitalThreshhold_in;
        ccell = ccell_in;
        vover = vbias_in - vbr_in;
        Vchr = Vchr_in;
        PDE_max = PDE_max_in;
        microcellTimes = vector<double>(numMicrocell, 0.0);
        microcellVoltages = vector<double>(numMicrocell, 0.0);
        precalculate_LUT();
    }

    // convert overvoltage to PDE
    inline double pde_from_volt(double overvoltage)
    {
        return PDE_max * (1 - exp(-(overvoltage / Vchr)));
    }

    // convert time since last detection to PDE
    inline double pde_from_time(double time)
    {
        double v = volt_from_time(time);
        return pde_from_volt(v);
    }

    // convert time since last detection to microcell voltage
    inline double volt_from_time(double time)
    {
        return vover * (1 - exp(-time / tauRecovery));
    }

    // Simulation function - takes as an argument a 'light' vector
    // light vector is the expected number of photons to strike the SiPM in simulation timestep dt.
    vector<double> simulate(vector<double> light)
    {
        vector<double> qFired = {};
        double pctdone;
        init_spads();
        // O(light.size()* numMicrocell)
        for (int i = 0; i < light.size(); i++)
        {
#ifndef NO_OUTPUT
            if ((i % 100 == 0 || i == (light.size() - 1)))
            {
                pctdone = (double)i / (double)(light.size() - 1);
                print_progress(pctdone);
            }
#endif
            qFired.push_back(selective_recharge_illuminate_LUT(light[i]));
        }
        return qFired;
    }

    // "Full" simulation function - takes as an argument a 'light' vector
    // "Full" simulation simulates every single microcell rather than using a Poisson PDE to randomly distribute photons. Slow and obselete.
    // light vector is the expected number of photons to strike the SiPM in simulation timestep dt.
    vector<double> simulate_full(vector<double> light)
    {
        vector<double> qFired = {};
        double l;
        double pctdone;
        init_spads();
        // O(light.size()* numMicrocell)
        for (int i = 0; i < light.size(); i++)
        {
            if (i % 100 == 0 || i == (light.size() - 1))
            {
                pctdone = (double)i / (double)(light.size() - 1);
                print_progress(pctdone);
            }
            l = light[i];
            qFired.push_back(recharge_illuminate_LUT(l));
        }
        return qFired;
    }

private:
    // "Hidden Variables" of live SiPM state
    vector<double> microcellTimes;    // Times since last detected photon
    vector<double> microcellVoltages; // Instantaneous Voltage for each microcell

    //// RANDOM NUMBER GENERATORS

    default_random_engine poissonEngine; // generator for Poisson Distribution
    mt19937_64 unifRandomEngine;
    uniform_real_distribution<double> unif;

    // random double between range a and b
    double unif_rand_double(double a, double b)
    {
        return unif(unifRandomEngine) * (b - a) + a;
    }

    // Uniform random integer. Do not change - this is fast
    int unif_rand_int(int a, int b)
    {
        return (int)(a + static_cast<double>(rand()) / (static_cast<double>(RAND_MAX / (b - a))));
    }

    //// SIMULATION METHODS

    void init_spads(void)
    {
        for (int i = 0; i < numMicrocell; i++)
        {
            microcellTimes[i] = tauRecovery * unif_rand_double(0, 10);
        }
    }

    double selective_recharge_illuminate_LUT(double photonsPerSecond)
    {
        double output = 0;
        double volt = 0;

        // randomly sample poisson parameter lambda input to generate number of incoming photons
        poisson_distribution<int> distribution(photonsPerSecond);
        int poissPhotons = distribution(poissonEngine);

        // generate n random microcells to strike - n random microcells
        vector<int> struckMicrocells = {};
        for (int i = 0; i < poissPhotons; i++)
        {
            struckMicrocells.push_back(unif_rand_int(0, numMicrocell));
        }

        // recharge all microcells
        for (int i = 0; i < numMicrocell; i++)
        {
            microcellTimes[i] += dt;
        }

        for (auto &i : struckMicrocells)
        {
            if (unif_rand_double(0, 1) < (pde_LUT(microcellTimes[i])))
            {
                volt = volt_LUT(microcellTimes[i]);
                microcellTimes[i] = 0;
                if (volt > digitalThreshhold * vover)
                {
                    output += volt * ccell;
                }
            }
        }
        return output;
    }

    // "full simulation" - does not use Poisson Stats, approximates with a uniform distribution
    double recharge_illuminate_LUT(double photonsPerSecond)
    {
        double output = 0;
        double volt = 0;
        for (int i = 0; i < numMicrocell; i++)
        {
            microcellTimes[i] += dt;
            if (unif_rand_double(0, 1) < (pde_LUT(microcellTimes[i]) * (photonsPerSecond / numMicrocell)))
            {
                volt = volt_LUT(microcellTimes[i]);
                microcellTimes[i] = 0;
                if (volt > digitalThreshhold * vover)
                {
                    output += volt * ccell;
                }
            }
        }
        return output;
    }

    // "full simulation" - does not use Poisson Stats, approximates with a uniform distribution
    // no lookup table
    double recharge_illuminate(double photonsPerSecond)
    {
        double output = 0;
        double volt = 0;
        for (int i = 0; i < numMicrocell; i++)
        {
            microcellTimes[i] += dt;
            volt = volt_from_time(microcellTimes[i]);
            if (unif_rand_double(0, 1) < (pde_from_volt(volt) * (photonsPerSecond / numMicrocell)))
            {
                microcellTimes[i] = 0;
                if (volt > digitalThreshhold * vover)
                {
                    output += volt * ccell;
                }
            }
        }
        return output;
    }

    //// UTILITY FUNCTIONS

    // progress bar
    void print_progress(double percentage)
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
    void test_rand_funcs()
    {
        const int nstars = 100;    // maximum number of stars to distribute
        const int nintervals = 10; // number of intervals
        int iters = 10000;

        cout << "\n\n***** UNIFORM *****\n"
             << endl;
        double s1;
        double test1 = 0;
        int p1[nintervals] = {};
        for (int i = 0; i < iters; i++)
        {
            s1 = unif_rand_double(0, 1);
            test1 += s1;
            ++p1[int(nintervals * s1)];
        }
        cout << "mean dist 1: " << test1 / iters << endl;

        for (int i = 0; i < nintervals; i++)
        {
            cout << float(i) / nintervals << "-" << float(i + 1) / nintervals << "\t: ";
            cout << string(p1[i] * nstars / iters, '*') << endl;
        }

        double lambda = 3.5;
        const int poiss_nintervals = 20;
        int p2[poiss_nintervals] = {};
        int s2;
        for (int i = 0; i < iters; i++)
        {
            poisson_distribution<int> distribution(5);
            s2 = distribution(poissonEngine);
            ++p2[int(s2)];
        }

        cout << "\n\n***** POISSON *****\n"
             << endl;
        for (int i = 0; i < poiss_nintervals; i++)
        {
            cout << i << "\t: ";
            cout << string(p2[i] * nstars / iters, '*') << endl;
        }
    }

    //// LOOKUP TABLE PARAMS AND FUNCTIONS

    static const size_t LUTSize = 15;
    double tVecLUT[LUTSize] = {0};
    double pdeVecLUT[LUTSize] = {0};
    double vVecLUT[LUTSize] = {0};

    void precalculate_LUT(void)
    {
        int numpoint = (int)LUTSize;
        double maxt = 5.3 * tauRecovery;
        double ddt = (double)maxt / numpoint;
        for (int i = 0; i < numpoint; i++)
        {
            tVecLUT[i] = i * ddt;
            vVecLUT[i] = vover * (1 - exp(-tVecLUT[i] / tauRecovery));
            pdeVecLUT[i] = pde_from_volt(vVecLUT[i]);
        }
    }

    // photon detection efficiency as a function of time lookup table
    double pde_LUT(double x)
    {
        return LUT(x, pdeVecLUT);
    }

    // ucell voltage as a function of time lookup table
    double volt_LUT(double x)
    {
        return LUT(x, vVecLUT);
    }

    // define a generic lookup table that works on with the time vector
    double LUT(double x, double *workingVector)
    {
        double *xs = tVecLUT;
        double *ys = workingVector;
        // number of elements in the array
        const int count = (const int)LUTSize;
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
};
