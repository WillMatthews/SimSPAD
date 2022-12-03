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
#include <algorithm>
#include <random>

// Progress bar defines
#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

class SiPM
{
public:
    int numMicrocell;
    double vBias;
    double vBr;
    double vOver;
    double tauRecovery;
    double digitalThreshold;
    double cCell;
    double dt;
    double vChr;
    double pdeMax;
    double tauFwhm;

    SiPM(int numMicrocell_in, double vBias_in, double vBr_in, double tauRecovery_in, double tauFwhm_in, double digitalThreshold_in, double ccell_in, double Vchr_in, double PDE_max_in);

    SiPM(int numMicrocell_in, double vBias_in, double vBr_in, double tauRecovery_in, double digitalThreshold_in, double ccell_in, double Vchr_in, double PDE_max_in);

    SiPM(std::vector<double> svars);

    SiPM();

    ~SiPM();

    inline double pde_from_volt(double overvoltage);

    inline double pde_from_time(double time);

    inline double volt_from_time(double time);

    std::vector<double> simulate(std::vector<double> light, bool silent);

    std::vector<double> simulate_full(std::vector<double> light);

    std::vector<double> shape_output(std::vector<double> inputVec);

private:
    std::vector<double> microcellTimes;

    std::mt19937_64 poissonEngine;
    std::mt19937_64 unifRandomEngine;
    std::mt19937_64 exponentialEngine;
    std::uniform_real_distribution<double> unif;

    void seed_engines(void);

    double unif_rand_double(double a, double b);

    int unif_rand_int(int a, int b);

    void init_spads(std::vector<double> light);

    double selective_recharge_illuminate_LUT(double T, double photonsPerDt);

    double recharge_illuminate(double photonsPerSecond);

    void print_progress(double percentage) const;

    void test_rand_funcs();

    void input_sanitation(void);

    int LUTSize;
    double *tVecLUT;
    double *pdeVecLUT;
    double *vVecLUT;

    void precalculate_LUT(void);

    double pde_LUT(double x) const;

    double volt_LUT(double x) const;

    double LUT(double x, double *workingVector) const;

    double trapezoidal(double (SiPM::*f)(double), double lower, double upper, int n);
};

#endif // SIPM_H
