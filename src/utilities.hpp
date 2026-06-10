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

#ifndef UTILITIES_H
#define UTILITIES_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include <tuple>
#include <cstddef>
#include <map>
#include "sipm.hpp"

std::tuple<std::vector<double>, SiPM> load_binary(std::string filename);

void write_binary(std::string filename, SiPM sipm, std::vector<double> response);

// ---------------------------------------------------------------------------
// Self-describing IO: NumPy .npy for the (bulk) waveform, flat JSON for the
// (tiny) device parameters. The waveform array is 1-D little-endian float64.
// Both reader and writer stream in chunks so arbitrarily long traces can be
// processed in bounded memory.
// ---------------------------------------------------------------------------

// Streaming reader for a 1-D little-endian float64 .npy file.
class NpyReader
{
public:
    explicit NpyReader(const std::string &filename);
    std::size_t count() const { return nElems; } // total number of doubles
    std::size_t read(double *buf, std::size_t n); // read up to n; returns count read
    void rewind();                                // seek back to the first sample
private:
    std::ifstream fin;
    std::size_t nElems;
    std::streampos dataStart;
};

// Streaming writer for a 1-D little-endian float64 .npy file. `count` (the
// final sample count) must be known up-front so a valid header can be written
// before the body -- which it always is here, since the SiPM transform is
// length-preserving.
class NpyWriter
{
public:
    NpyWriter(const std::string &filename, std::size_t count);
    void write(const double *buf, std::size_t n);
    void close();
private:
    std::ofstream fout;
};

// Flat-JSON device parameters <-> SiPM.
std::map<std::string, double> parse_flat_json(const std::string &text);
SiPM load_params_json(const std::string &filename);
std::string sipm_to_json(SiPM &sipm);
void save_params_json(const std::string &filename, SiPM &sipm);

std::vector<double> conv1d(std::vector<double> inputVec, std::vector<double> kernel);

std::vector<double> get_gaussian(double dt, double tauFwhm);

std::tuple<std::wstring, double> exponent_val(double num);

template <typename T>
std::vector<double> linspace(T start_in, T end_in, unsigned int num_in);

#endif // UTILITIES_H
