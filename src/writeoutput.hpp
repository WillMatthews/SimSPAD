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
#include <cmath>
#include <chrono>
#include <ctime>

void write_vector_to_file(const std::vector<double>& myVector, std::string filename)
{

    std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
    std::time_t tt;
    tt = std::chrono::system_clock::to_time_t(currentTime);
    string timeStr = ctime(tt);

    std::ofstream ofs(filename, std::ios::out | std::ofstream::binary);
    ofs << "SIMSPAD OUTPUT\n";
    ofs << "dt:" << dt << "\n"
    ofs << "

    std::ostream_iterator<char> osi{ ofs };

    const char* beginByte = (char*)&myVector[0];
    const char* endByte = (char*)&myVector.back() + sizeof(double);
    std::copy(beginByte, endByte, osi);
}

