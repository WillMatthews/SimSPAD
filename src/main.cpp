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

//#define NO_OUTPUT

#include <iostream>
#include <vector>
#include <string>
#include <iterator>
#include <cmath>
#include <random>
#include <chrono>
//#include <thread>
#include <ctime>
#include "sipm.hpp"
#include "utilities.hpp"

using namespace std;

// create lambda expression for a simulation run (input csv -> output csv).
// might be helpful if multithreading in the future
auto sim_lambda = [](string fname)
{
    vector<double> out = {};
    vector<double> in = {};

    // Generate Dummy Input for Testing //
    // int ber_run_samples = 844759;
    // vector<double> in(ber_run_samples, 10); // DC light source
    // dt = 1E-10;
    //////////////////////////////////////

    // SiPM j30020(14410, 27.5, 24.5, 2.2 * 14E-9, 0.0, 4.6e-14, 2.04, 0.46);

    SiPM sipm;

    tie(in, sipm) = loadBinary(fname + ".bin");

    auto start = chrono::steady_clock::now();

    out = sipm.simulate(in);
    // out = j30020.simulate_full(in);
    // j30020.test_rand_funcs();

    auto end = chrono::steady_clock::now();
    chrono::duration<double> elapsed = end - start;

    // write_vector_to_file(const std::vector<double>& myVector, std::string filename)

#ifndef NO_OUTPUT
    print_info(elapsed, sipm.dt, out, sipm.numMicrocell);
#endif
};

// Run simulation and time
int main(int argc, char *argv[])
{
    unsigned int time_ui = static_cast<unsigned int>(time(NULL));
    srand(time_ui);

    (void)argc;
    (void)argv;

    ios::sync_with_stdio(0);
    cin.tie(0);

    sim_lambda("sipm");

    return EXIT_SUCCESS;
}
