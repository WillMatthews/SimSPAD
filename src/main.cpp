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
#include <cstdio>
#include "sipm.hpp"
#include "utilities.hpp"

using namespace std;

// Print end of simulation run information for user / debug
void print_info(chrono::duration<double> elapsed, SiPM &sipm, size_t inputSize, double sumOut)
{
    int numMicrocell = sipm.numMicrocell;
    double dt = sipm.dt;
    auto sysclock = chrono::system_clock::now();
    auto tt = chrono::system_clock::to_time_t(sysclock);
    string timeStr = ctime(&tt);

    if (inputSize == 0)
    {
        return;
    }

    locale::global(locale("en_US.utf8"));
    wcout.imbue(locale());

    cout << "Simulation Run at: " << timeStr << endl;

    wstring prefix;
    double val;
    tie(prefix, val) = exponent_val(elapsed.count());
    wcout << "Elapsed Time:\t\t" << val << " " << prefix << "s" << endl;
    tie(prefix, val) = exponent_val(dt * (double)inputSize);
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

    double Ibias = sumOut / ((double)inputSize * dt); // Calculate the bias current
    tie(prefix, val) = exponent_val(Ibias);
    wcout << "Simulated Ibias:\t" << val << " " << prefix << "A" << endl;
}

// Run a simulation streaming a .npy waveform through the SiPM in bounded
// memory: JSON device parameters + .npy light in -> .npy charge out. The
// transform is length-preserving, so the output header is written before its
// body. Two passes over the (paged) input: one to seed the initial microcell
// age distribution from the mean light level, one to simulate.
void simulate(string params_file, string fname_in, string fname_out, bool silence)
{
    const size_t chunk = 1u << 16; // 65536 samples per block

    SiPM sipm = load_params_json(params_file);
    NpyReader reader(fname_in);
    size_t N = reader.count();
    NpyWriter writer(fname_out, N);

    // Pass 1: mean photons/dt over the whole trace (raw, matching the old
    // in-memory init_spads) to seed the initial age distribution.
    double rawSum = 0.0;
    {
        vector<double> buf(chunk);
        size_t got;
        while ((got = reader.read(buf.data(), chunk)) > 0)
        {
            for (size_t i = 0; i < got; i++)
            {
                rawSum += buf[i];
            }
        }
    }
    double mean = N ? rawSum / (double)N : 0.0;
    sipm.init_state(mean, (unsigned long)N);

    // Pass 2: stream the simulation, writing each output block as it is made.
    reader.rewind();
    auto start = chrono::steady_clock::now();
    double outSum = 0.0;
    {
        vector<double> inbuf(chunk), outbuf(chunk);
        size_t got, done = 0;
        while ((got = reader.read(inbuf.data(), chunk)) > 0)
        {
            sipm.simulate_chunk(inbuf.data(), outbuf.data(), got);
            writer.write(outbuf.data(), got);
            for (size_t i = 0; i < got; i++)
            {
                outSum += outbuf[i];
            }
            done += got;
            if (!silence && N)
            {
                fprintf(stderr, "\r  simulating... %5.1f%%", 100.0 * (double)done / (double)N);
            }
        }
        if (!silence && N)
        {
            fprintf(stderr, "\r  simulating... done   \n");
        }
    }
    writer.close();
    auto end = chrono::steady_clock::now();

    chrono::duration<double> elapsed = end - start;

    if (!silence)
    {
        print_info(elapsed, sipm, N, outSum);
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
    cerr << "Usage: " << name << " -p PARAMS.json -i LIGHT.npy -o OUT.npy <option(s)>\n"
         << "Reads device parameters from a flat JSON file and the optical input\n"
         << "from a 1-D float64 .npy file, streams the simulation, and writes the\n"
         << "charge-per-step response to a 1-D float64 .npy file.\n\n"
         << "Options:\n"
         << "\t-h,--help\t\tShow this help message\n"
         << "\t-s,--silent\t\tSilence output\n"
         << "\t-v,--version\t\tPrint SimSPAD version number\n"
         << "\t-p,--params PARAMS\tDevice parameters (.json) [required]\n"
         << "\t-i,--input INPUT\tOptical input waveform (.npy) [required]\n"
         << "\t-o,--output OUTPUT\tResponse output path (.npy) [required]"
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

    string params = "";
    string source = "";
    string destination = "";
    bool silence = false;

    // Small helper to consume an option's argument.
    auto take_arg = [&](int &i, const char *opt) -> const char * {
        if (i + 1 < argc)
        {
            return argv[++i]; // consume and skip the value
        }
        cerr << opt << " option requires one argument." << endl;
        return nullptr;
    };

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
        else if ((arg == "-p") || (arg == "--params"))
        {
            const char *a = take_arg(i, "--params");
            if (!a)
                return EXIT_FAILURE;
            params = a;
        }
        else if ((arg == "-i") || (arg == "--input"))
        {
            const char *a = take_arg(i, "--input");
            if (!a)
                return EXIT_FAILURE;
            source = a;
        }
        else if ((arg == "-o") || (arg == "--output"))
        {
            const char *a = take_arg(i, "--output");
            if (!a)
                return EXIT_FAILURE;
            destination = a;
        }
        else
        {
            source = argv[i]; // bare positional argument is the input waveform
        }
    }

    if (params.empty() || source.empty() || destination.empty())
    {
        cerr << "error: --params, --input and --output are all required." << endl;
        show_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (!silence)
    {
        cli_logo();
    }

    try
    {
        simulate(params, source, destination, silence);
    }
    catch (const std::exception &e)
    {
        cerr << "error: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
