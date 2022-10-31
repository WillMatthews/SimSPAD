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
#include "sipm.hpp"

std::tuple<std::vector<double>, SiPM> load_binary(std::string filename);

void write_binary(std::string filename, SiPM sipm, std::vector<double> response);

std::vector<double> conv1d(std::vector<double> inputVec, std::vector<double> kernel);

std::vector<double> get_gaussian(double dt, double tauFwhm);

std::tuple<std::wstring, double> exponent_val(double num);

void print_info(std::chrono::duration<double> elapsed, double dt, std::vector<double> outputVec, int numMicrocell);

#endif // UTILITIES_H
