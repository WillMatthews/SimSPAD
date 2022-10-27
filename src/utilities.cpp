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
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include <tuple>
#include "sipm.hpp"

using namespace std;

// load experimental run data from a binary file (packaged from MATLAB)
/*
FILE FORMAT:
    double dt = x(0);
    double num_microcell = x(1);
    double vbias = x(2);
    double vbr = x(3);
    double recovery = x(4);
    double pde_Max = x(5);
    double pde_vchr = x(6);
    double ccell = x(7);
    double pulse_fwhm = x(8);
    double digital_threshholds = x(9);

    vector<double> optical_input = x(10:end);
*/
tuple<vector<double>, SiPM> loadBinary(string filename)
{
    vector<double> optical_input = {};
    ifstream fin(filename, ios::binary);
    vector<double> sipmvars = {};
    int i = 0;
    for (double read; fin.read(reinterpret_cast<char *>(&read), sizeof(read));)
    {
        if (i < 10)
        {
            sipmvars.push_back(read);
        }
        else
        {
            optical_input.push_back(read);
        }
        ++i;
    }

    return make_tuple(optical_input, SiPM(sipmvars));
}

// write binary file from simulation settings and output
void writeBinary(string filename, SiPM sipm, vector<double> response)
{
    ofstream fout(filename, ios::out | ios::binary);

    /*
    chrono::system_clock::time_point currentTime = chrono::system_clock::now();
    time_t tt;
    tt = chrono::system_clock::to_time_t(currentTime);
    string timeStr = ctime(tt);
    */

    vector<double> sipm_params = {};
    sipm_params.push_back(sipm.dt);
    sipm_params.push_back((double)sipm.numMicrocell);
    sipm_params.push_back(sipm.vBias);
    sipm_params.push_back(sipm.vBr);
    sipm_params.push_back(sipm.tauRecovery);
    sipm_params.push_back(sipm.pdeMax);
    sipm_params.push_back(sipm.vChr);
    sipm_params.push_back(sipm.cCell);
    sipm_params.push_back(sipm.tauFwhm);
    sipm_params.push_back(sipm.digitalThreshhold);

    sipm_params.insert(sipm_params.end(), response.begin(), response.end());

    fout.write((char *)(&sipm_params[0]), sizeof(sipm_params) * sipm_params.size());
    fout.close();
}

// output convolve to pulse shape using a gaussian approximation
vector<double> conv1d(vector<double> inputVec, vector<double> kernel)
{

    if (kernel.size() <= 1)
    {
        return inputVec;
    }

    int kernelsize = (int)kernel.size();

    if (kernelsize == 1)
    {
        return inputVec;
    }

    vector<double> outputVec = {};
    int inpoint;

    for (int i = 0; i < (int)inputVec.size(); i++)
    {
        outputVec.push_back(0.0);
        for (int j = 0; j < kernelsize; j++)
        {
            inpoint = i + j - kernelsize / 2;
            if (!(inpoint < 0 || (inpoint > (long)inputVec.size())))
            {
                outputVec[i] += kernel[j] * inputVec[inpoint];
            }
        }
    }
    return outputVec;
}

// generate gaussian kernel
// TODO check if the pulse width is correct!
vector<double> get_gaussian(double dt, double tauFwhm)
{
    const double gauconst = (double)(1 / sqrt(2 * M_PI));
    const double fwhmConversionConst = sqrt(2 * log(2)) / 2;
    const double sigma = (tauFwhm / dt) / fwhmConversionConst;
    const double numSigma = 4.0;

    int gau_numpoint = (int)ceil(numSigma * sigma);

    vector<double> kernel = {};
    if (gau_numpoint == 1)
    {
        kernel.push_back(1);
        return kernel;
    }

    double gaupower;

    for (int i = -(gau_numpoint - 1); i < (gau_numpoint - 1); i++)
    {
        gaupower = -pow((double)i / sigma, 2) / 2;
        kernel.push_back(gauconst * exp(gaupower));
    }
    return kernel;
}

// Given a number `num` scale to engineering notation (nearest power of three) and return the unit prefix associated with the unit
tuple<wstring, double> exponent_val(double num)
{
    int floor_prefix = floor(log10(num));
    wstring prefixes[9] = {L"f", L"p", L"n", L"μ", L"m", L"", L"k", L"M", L"G"};
    bool triggered = false;
    int k = 0;
    for (int i = -15; i < 10; i = i + 3)
    {
        if (floor_prefix < i)
        {
            triggered = true;
            k--;
            break;
        };
        k++;
    }
    if (!triggered)
    {
        return make_tuple(prefixes[5], num);
    }
    return make_tuple(prefixes[k], num * pow(10, -(k * 3 - 15)));
}

void print_info(chrono::duration<double> elapsed, double dt, vector<double> outvec, int numMicrocell)
{
    auto sysclock = chrono::system_clock::now();
    auto tt = chrono::system_clock::to_time_t(sysclock);
    string timeStr = ctime(&tt);

    int inputsize = outvec.size();

    locale::global(locale("en_US.utf8"));
    wcout.imbue(locale());

    cout << "Simulation Run at: " << timeStr << endl;

    wstring prefix;
    double val;
    tie(prefix, val) = exponent_val(elapsed.count());
    wcout << "Elapsed Time:\t\t" << val << prefix << "s" << endl;
    tie(prefix, val) = exponent_val(dt * inputsize);
    wcout << "Simulated Time:\t\t" << val << prefix << "s" << endl;
    tie(prefix, val) = exponent_val(dt);
    wcout << "Simulation dt:\t\t" << val << prefix << "s" << endl;
    cout << "Time Steps:\t\t" << inputsize << "Sa" << endl;
    double time_per_iter = ((double)elapsed.count()) / ((double)inputsize);
    tie(prefix, val) = exponent_val(time_per_iter);
    wcout << "Compute Per Step:\t" << val << prefix << "s" << endl;
    tie(prefix, val) = exponent_val(time_per_iter / numMicrocell);
    wcout << "Compute Per uCell Step: " << val << prefix << "s" << endl;

    double sumOut = 0;
    for (int i = 0; i < inputsize; i++)
    {
        sumOut += outvec[i];
    }
    double Ibias = sumOut / (inputsize * dt);
    cout << "Simulated Ibias:\t" << Ibias * 1E3 << "mA" << endl;
}
