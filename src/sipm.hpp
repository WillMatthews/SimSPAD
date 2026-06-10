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

// Upper bound on the microcell count accepted from untrusted input. Real SiPMs top
// out well below this (the largest catalogue arrays are ~1e6 cells); the cap bounds
// the microcellTimes allocation and the O(N*numMicrocell) simulation cost so a
// crafted parameter set cannot exhaust memory/CPU (GHSA-c79g-qphv-xjxh, GHSA-f2ph-wv99-c83q).
constexpr unsigned long MAX_MICROCELL = 10000000UL; // 1e7

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

    // Streaming core. init_state() prepares the microcell ages once (given the
    // mean photons/dt striking the whole array, used only to seed the initial
    // age distribution) and resets the simulation clock. simulate_chunk() then
    // advances the state by `n` samples, reading `in[0..n)` and writing
    // `out[0..n)`, and may be called repeatedly to stream arbitrarily long
    // traces with O(n + numMicrocell) memory. simulate(vector) is a thin
    // wrapper over these two.
    void init_state(double meanPhotonsPerDt, unsigned long nSteps);

    void simulate_chunk(const double *in, double *out, std::size_t n);

    std::vector<double> shape_output(std::vector<double> inputVec);

private:
    std::vector<double> microcellTimes;
    double simClock = 0.0; // running simulation time, carried across chunks

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
    std::vector<double> tVecLUT;
    std::vector<double> pdeVecLUT;
    std::vector<double> vVecLUT;

    void precalculate_LUT(void);

    double pde_LUT(double x) const;

    double volt_LUT(double x) const;

    double LUT(double x, const double *workingVector) const;

    // trapezoidal integrations over a function
    double trapezoidal(double (SiPM::*f)(double), double lower, double upper, unsigned long n);
    std::vector<double> cum_trapezoidal(double (SiPM::*f)(double), double lower, double upper, unsigned long n);

    // trapezoidal integrations over a vector
    double trapezoidal(std::vector<double>, double);
    std::vector<double> cum_trapezoidal(std::vector<double>, double);
};

#endif // SIPM_H
