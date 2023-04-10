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

// Print end of simulation run information for user / debug
void print_info(chrono::duration<double> elapsed, SiPM sipm, vector<double> outputVec)
{
    int numMicrocell = sipm.numMicrocell;
    double dt = sipm.dt;
    auto sysclock = chrono::system_clock::now();
    auto tt = chrono::system_clock::to_time_t(sysclock);
    string timeStr = ctime(&tt);

    int inputSize = outputVec.size();

    locale::global(locale("en_US.utf8"));
    wcout.imbue(locale());

    cout << "Simulation Run at: " << timeStr << endl;

    wstring prefix;
    double val;
    tie(prefix, val) = exponent_val(elapsed.count());
    wcout << "Elapsed Time:\t\t" << val << " " << prefix << "s" << endl;
    tie(prefix, val) = exponent_val(dt * inputSize);
    wcout << "Simulated Time:\t\t" << val << " " << prefix << "s" << endl;
    tie(prefix, val) = exponent_val(dt);
    wcout << "Simulation dt:\t\t" << val << " " << prefix << "s" << endl;
    cout << "Time Steps:\t\t" << inputSize << " "
         << "Sa" << endl;
    double time_per_iter = ((double)elapsed.count()) / ((double)inputSize);
    tie(prefix, val) = exponent_val(time_per_iter);
    wcout << "Compute Per Step:\t" << val << " " << prefix << "s" << endl;
    tie(prefix, val) = exponent_val(time_per_iter / numMicrocell);
    wcout << "Compute Per uCell Step: " << val << " " << prefix << "s" << endl;

    double sumOut = 0; // Sum of all the charge from the SiPM from the experiment
    if (!outputVec.empty())
    {
        sumOut = reduce(outputVec.begin(), outputVec.end()); // Sum all responses
    }
    double Ibias = sumOut / (inputSize * dt); // Calculate the bias current
    tie(prefix, val) = exponent_val(Ibias);
    wcout << "Simulated Ibias:\t" << val << " " << prefix << "A" << endl;
}

// Create lambda expression for a simulation run (binary in -> binary out).
// Might be helpful if multi-threading in the future
void simulate(string fname_in, string fname_out, bool silence)
{
    vector<double> out = {};
    vector<double> in = {};
    SiPM sipm;

    tie(in, sipm) = load_binary(fname_in);
    auto start = chrono::steady_clock::now();

    out = sipm.simulate(in, silence);

    auto end = chrono::steady_clock::now();

    // vector<double> out2 = sipm.shape_output(out);

    chrono::duration<double> elapsed = end - start;

    write_binary(fname_out, sipm, out);

    if (!silence)
    {
        print_info(elapsed, sipm, out);
    }
}

// Print version number
static void print_version()
{
    cout << "SimSPAD " << VERSION << endl;
}

// Print help text
static void show_usage(string name)
{
    cerr << "Usage: " << name << " <option(s)> INPUT "
         << "Options:\n"
         << "\t-h,--help\t\tShow this help message\n"
         << "\t-s,--silent\t\tSilence output\n"
         << "\t-v,--version\t\tPrint SimSPAD version number\n"
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

    string arg;

    if (argc == 2)
    {
        arg = argv[1];
        if ((arg == "-v") || (arg == "--version"))
        {
            print_version();
            return EXIT_SUCCESS;
        }
    }

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
        arg = argv[i];
        if ((arg == "-h") || (arg == "--help"))
        {
            show_usage(argv[0]);
            return EXIT_SUCCESS;
        }
        else if ((arg == "-s") || (arg == "--silent"))
        {
            silence = true;
        }
        else if ((arg == "-v") || (arg == "--version"))
        {
            print_version();
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
                return EXIT_FAILURE;
            }
        }
        else
        {
            source = argv[i];
        }
    }

    if (!silence)
    {
        cli_logo();
    }

    simulate(source, destination, silence);

    return EXIT_SUCCESS;
}
