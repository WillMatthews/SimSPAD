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

// Load experimental run data from a binary file (packaged from MATLAB)
/*
FILE FORMAT:
    double dt = x(0);
    double num_microcell = x(1);
    double vbias = x(2);
    double vbr = x(3);
    double recovery = x(4);
    double pde_Max = x(5);
    double pde_Vchr = x(6);
    double ccell = x(7);
    double pulse_fwhm = x(8);
    double digital_thresholds = x(9);

    vector<double> optical_input = x(10:end);
*/
tuple<vector<double>, SiPM> load_binary(string filename)
{
    vector<double> optical_input = {};
    ifstream fin(filename, ios::binary);
    vector<double> sipmParametersVector = {};
    int i = 0;
    for (double read; fin.read(reinterpret_cast<char *>(&read), sizeof(read));)
    {
        if (i < 10)
        {
            sipmParametersVector.push_back(read);
        }
        else
        {
            optical_input.push_back(read);
        }
        ++i;
    }

    return make_tuple(optical_input, SiPM(sipmParametersVector));
}

// Write binary file from simulation settings and output
void write_binary(string filename, SiPM sipm, vector<double> response)
{
    ofstream fout(filename, ios::out | ios::binary);

    /*
    chrono::system_clock::time_point currentTime = chrono::system_clock::now();
    time_t tt;
    tt = chrono::system_clock::to_time_t(currentTime);
    string timeStr = ctime(tt);
    */

    vector<double> sipmParametersVector = {};
    sipmParametersVector.push_back(sipm.dt);
    sipmParametersVector.push_back((double)sipm.numMicrocell);
    sipmParametersVector.push_back(sipm.vBias);
    sipmParametersVector.push_back(sipm.vBr);
    sipmParametersVector.push_back(sipm.tauRecovery);
    sipmParametersVector.push_back(sipm.pdeMax);
    sipmParametersVector.push_back(sipm.vChr);
    sipmParametersVector.push_back(sipm.cCell);
    sipmParametersVector.push_back(sipm.tauFwhm);
    sipmParametersVector.push_back(sipm.digitalThreshold);

    // Concatenate response on the end of SiPM parameters vector
    sipmParametersVector.insert(sipmParametersVector.end(), response.begin(), response.end());

    fout.write((char *)(&sipmParametersVector[0]), sizeof(sipmParametersVector) * sipmParametersVector.size());
    fout.close();
}

// Output convolve to pulse shape using a gaussian approximation
vector<double> conv1d(vector<double> inputVec, vector<double> kernel)
{

    if (kernel.size() <= 1)
    {
        return inputVec;
    }

    int kernelSize = (int)kernel.size();

    if (kernelSize == 1)
    {
        return inputVec;
    }

    vector<double> outputVec = {};
    outputVec.reserve(inputVec.size());
    int innerPosition;

    for (int i = 0; i < (int)inputVec.size(); i++)
    {
        outputVec.push_back(0.0);
        for (int j = 0; j < kernelSize; j++)
        {
            innerPosition = i + j - kernelSize / 2;
            if (!(innerPosition < 0 || (innerPosition > (long)inputVec.size())))
            {
                outputVec[i] += kernel[j] * inputVec[innerPosition];
            }
        }
    }
    return outputVec;
}

// Generate Gaussian kernel
// TODO check if the pulse width is correct!
vector<double> get_gaussian(double dt, double tauFwhm)
{
    const double gaussianConstant = (double)(1 / sqrt(2 * M_PI));
    const double fwhmConversionConst = sqrt(2 * log(2)) / 2;
    const double sigma = (tauFwhm / dt) / fwhmConversionConst;
    const double numSigma = 4.0;

    int gaussianNumberOfPoints = (int)ceil(numSigma * sigma);

    vector<double> kernel = {};
    kernel.reserve(gaussianNumberOfPoints * 2);

    if (gaussianNumberOfPoints == 1)
    {
        kernel.push_back(1);
        return kernel;
    }

    double gaussianPower;

    for (int i = -(gaussianNumberOfPoints - 1); i < (gaussianNumberOfPoints - 1); i++)
    {
        gaussianPower = -pow((double)i / sigma, 2) / 2;
        kernel.push_back(gaussianConstant * exp(gaussianPower));
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

// Linearly space num_in points between values start_in and end_in
// Template removed as I am stupid and keep getting undefined reference errors.
// template <typename T>
// vector<double> linspace(T start_in, T end_in, int num_in)
vector<double> linspace(double start_in, double end_in, int num_in)
{

    vector<double> linspaced;
    linspaced.reserve(num_in);

    double start = static_cast<double>(start_in);
    double end = static_cast<double>(end_in);
    double num = static_cast<double>(num_in);

    if (num == 0)
    {
        return linspaced;
    }
    if (num == 1)
    {
        linspaced.push_back(start);
        return linspaced;
    }

    double delta = (end - start) / (num - 1);

    for (int i = 0; i < num - 1; ++i)
    {
        linspaced.push_back(start + delta * i);
    }
    linspaced.push_back(end); // I want to ensure that start and end
                              // are exactly the same as the input
    return linspaced;
}
