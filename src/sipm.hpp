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

// Version is obtained from git tag.
// This is done by the Makefile, which replaces the string "replace_with_version" with the output of "git describe --tags --always --dirty"
#ifndef VERSION
#define VERSION "unknown"
#endif

void cli_logo(void);

class SiPM
{
public:
    unsigned long numMicrocell;
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

    SiPM(unsigned long numMicrocell_in, double vBias_in, double vBr_in, double tauRecovery_in, double tauFwhm_in, double digitalThreshold_in, double ccell_in, double Vchr_in, double PDE_max_in);

    SiPM(unsigned long numMicrocell_in, double vBias_in, double vBr_in, double tauRecovery_in, double digitalThreshold_in, double ccell_in, double Vchr_in, double PDE_max_in);

    SiPM(std::vector<double> svars);

    SiPM();

    ~SiPM();

    std::vector<double> dump_configuration(void);

    inline double pde_from_volt(double overvoltage);

    inline double pde_from_time(double time);

    inline double volt_from_time(double time);

    std::vector<double> simulate(std::vector<double> light, bool silent);

    std::vector<double> shape_output(std::vector<double> inputVec);

private:
    std::vector<double> microcellTimes;

    std::mt19937_64 poissonEngine;
    std::mt19937_64 unifRandomEngine;
    std::mt19937_64 renewalEngine;
    std::uniform_real_distribution<double> unif;

    void seed_engines(void);

    double unif_rand_double(double a, double b);

    unsigned long unif_rand_int(unsigned long a, unsigned long b);

    void init_spads(std::vector<double> light);

    double simulate_microcells(double T, double photonsPerDt);

    void print_progress(double percentage) const;

    // void test_rand_funcs();

    void input_sanitation(void);

    unsigned int LUTSize;
    double *tVecLUT;
    double *pdeVecLUT;
    double *vVecLUT;

    void precalculate_LUT(void);

    double pde_LUT(double x) const;

    double volt_LUT(double x) const;

    double LUT(double x, double *workingVector) const;

    // trapezoidal integrations over a function
    double trapezoidal(double (SiPM::*f)(double), double lower, double upper, unsigned long n);
    std::vector<double> cum_trapezoidal(double (SiPM::*f)(double), double lower, double upper, unsigned long n);

    // trapezoidal integrations over a vector
    double trapezoidal(std::vector<double>, double);
    std::vector<double> cum_trapezoidal(std::vector<double>, double);
};

#endif // SIPM_H
