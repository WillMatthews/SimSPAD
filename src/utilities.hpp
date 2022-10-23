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
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <chrono>
#include <ctime>
#include <tuple>
// #include "../lib/rapidcsv/src/rapidcsv.h"
#include "sipm.hpp"

std::tuple<std::vector<double>, SiPM> loadBinary(std::string filename);

void writeBinary(std::string filename, SiPM sipm, std::vector<double> response);

// void write_vector_to_file(const vector<double>& vectorToSave, , string filename);

// std::tuple<std::vector<double>, double> readCSV(std::string fname);

std::tuple<std::wstring, double> exponent_val(double num);

void print_info(std::chrono::duration<double> elapsed, double dt, std::vector<double> outvec, int numMicrocell);

#endif // UTILITIES_H
