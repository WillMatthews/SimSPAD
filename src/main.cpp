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
#include <iterator>
#include <cmath>
#include <random>
#include <chrono>
#include <ctime>
#include "sipm.hpp"
#include "utilities.hpp"

using namespace std;

// create lambda expression for a simulation run (binary in -> binary out).
// might be helpful if multi-threading in the future
void simulate(string fname_in, string fname_out, bool silence)
{
    vector<double> out = {};
    vector<double> in = {};
    SiPM sipm;

    tie(in, sipm) = load_binary(fname_in);
    auto start = chrono::steady_clock::now();

    out = sipm.simulate(in,in, silence);

    // out = sipm.simulate_full(in);
    // sipm.test_rand_funcs();
    auto end = chrono::steady_clock::now();

    // vector<double> out2 = sipm.shape_output(out);

    chrono::duration<double> elapsed = end - start;

    write_binary(fname_out, sipm, out);

    if (!silence)
    {
        print_info(elapsed, sipm.dt, out, sipm.numMicrocell);
    }
}

static void show_usage(string name)
{
    cerr << "Usage: " << name << " <option(s)> INPUT "
         << "Options:\n"
         << "\t-h,--help\t\tShow this help message\n"
         << "\t-s,--silent\t\tSilence output\n"
         // << "\t-c,--csv\t\tOutput in CSV format\n"
         << "\t-o,--output DESTINATION\tSpecify the destination path"
         << endl;
}

// Run simulation and time
int main(int argc, char *argv[])
{
    unsigned int time_ui = static_cast<unsigned int>(time(NULL));
    srand(time_ui);

    ios::sync_with_stdio(0);
    cin.tie(0);

    if (argc < 3)
    {
        show_usage(argv[0]);
        return 1;
    }

    string source = "";
    string destination = "";
    bool silence = false;
    for (int i = 1; i < argc; ++i)
    {
        string arg = argv[i];
        if ((arg == "-h") || (arg == "--help"))
        {
            show_usage(argv[0]);
            return 0;
        }
        else if ((arg == "-s") || (arg == "--silent"))
        {
            silence = true;
        }
        else if ((arg == "-o") || (arg == "--output"))
        {
            if (i + 1 < argc)
            {                              // Make sure we aren't at the end of argv
                destination = argv[i + 1]; // Increment 'i' so we don't get the argument as the next argv[i].
            }
            else
            { // No argument to the destination option.
                cerr << "--destination option requires one argument." << endl;
                return 1;
            }
        }
        else
        {
            source = argv[i];
        }
    }

    if (!silence)
    {
        cout << "   _____ _          _____ ____  ___    ____ " << endl;
        cout << "  / ___/(_)___ ___ / ___// __ \\/   |  / __ \\" << endl;
        cout << "  \\__ \\/ / __ `__  \\__ \\/ /_/ / /| | / / / /" << endl;
        cout << " ___/ / / / / / / /__/ / ____/ ___ |/ /_/ / " << endl;
        cout << "/____/_/_/ /_/ /_/____/_/   /_/  |_/_____/  \n"
             << endl;
    }

    simulate(source, destination, silence);

    return EXIT_SUCCESS;
}
