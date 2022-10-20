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

#ifndef SIPM_H
#define SIPM_H

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

class SiPM
{
public:
    int numMicrocell;
    double vbias;
    double vbr;
    double vover;
    double tauRecovery;
    double digitalThreshhold;
    double ccell;
    double dt;
    double Vchr;
    double PDE_max;

    SiPM(int numMicrocell_in, double vbias_in, double vbr_in, double tauRecovery_in, double digitalThreshhold_in, double ccell_in, double Vchr_in, double PDE_max_in);

    ~SiPM();

    inline double pde_from_volt(double overvoltage);

    inline double pde_from_time(double time);

    inline double volt_from_time(double time);

    std::vector<double> simulate(std::vector<double> light);

    std::vector<double> simulate_full(std::vector<double> light);

private:
    std::vector<double> microcellTimes;

    std::default_random_engine poissonEngine;
    std::mt19937_64 unifRandomEngine;
    std::uniform_real_distribution<double> unif;

    double unif_rand_double(double a, double b);

    int unif_rand_int(int a, int b);

    void init_spads(void);

    double selective_recharge_illuminate_LUT(double photonsPerSecond);

    double recharge_illuminate_LUT(double photonsPerSecond);

    double recharge_illuminate(double photonsPerSecond);

    void print_progress(double percentage);

    void test_rand_funcs();

    //static const size_t LUTSize;
    int LUTSize;
    double* tVecLUT;
    double* pdeVecLUT;
    double* vVecLUT;

    void precalculate_LUT(void);

    double pde_LUT(double x);

    double volt_LUT(double x);

    double LUT(double x, double *workingVector);
};

#endif // SIPM_H
