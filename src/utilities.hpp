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


using namespace std;

/*
void write_vector_to_file(const vector<double>& myVector, string filename)
{

    chrono::system_clock::time_point currentTime = chrono::system_clock::now();
    time_t tt;
    tt = chrono::system_clock::to_time_t(currentTime);
    string timeStr = ctime(tt);

    ofstream ofs(filename, ios::out | ofstream::binary);
    ofs << "SIMSPAD OUTPUT\n";
    ofs << "dt:" << dt << "\n";
    ofs << ";

    ostream_iterator<char> osi{ ofs };

    const char* beginByte = (char*)&myVector[0];
    const char* endByte = (char*)&myVector.back() + sizeof(double);
    copy(beginByte, endByte, osi);
}
*/

tuple<wstring, double> exponent_val(double num){
    int floor_prefix = floor(log10(num));
    wstring prefixes[9] = {L"f", L"p", L"n", L"μ", L"m", L"", L"k", L"M", L"G"};
    bool triggered = false;
    int idx_prefix;
    int k = 0;
    for (int i=-15; i<10; i=i+3){
        if (floor_prefix < i){
            triggered = true;
            idx_prefix = k-1;
            break;
        };
        k = k+1;
    }
    if (!triggered){
        return make_tuple(prefixes[5], num);
    }
    double pot = (double) -(idx_prefix * 3 - 15);
    return make_tuple(prefixes[idx_prefix], num*pow(10,pot)); 
}


void print_info(chrono::duration<double> elapsed, double dt, vector<double>outvec, int numMicrocell){
    auto sysclock = chrono::system_clock::now();
    auto tt = chrono::system_clock::to_time_t(sysclock);
    string timeStr = ctime(&tt);

    int inputsize = outvec.size();

    locale::global(locale("en_US.utf8"));
    wcout.imbue(locale());

    cout << "\n\nSimulation Run at: " << timeStr << endl;

    wstring prefix;
    double val;
    tie(prefix, val) = exponent_val(elapsed.count());
    wcout << "Elapsed Time:\t " << val << prefix << "s" <<  endl;
    tie(prefix, val) = exponent_val(dt*inputsize);
    wcout << "Simulated Time:\t " << val << prefix << "s" << endl;
    tie(prefix, val) = exponent_val(dt);
    wcout << "Resolution:\t " << val << prefix << "s" << endl;
    cout << "Time Steps:\t " << inputsize << "Sa" <<  endl;
    double time_per_iter = ( (double) elapsed.count()) / ( (double) inputsize);
    tie(prefix, val) = exponent_val(time_per_iter);
    wcout << "Computer Time Per Step:\t " << val << prefix << "s" << endl;
    tie(prefix, val) = exponent_val(time_per_iter/numMicrocell);
    wcout << "Computer Time Per uCell Step:\t " << val << prefix  << "s" << endl;

    double sumOut = 0;
    for (int i = 0; i< inputsize; i++){
        sumOut += outvec[i];
    }
    double Ibias = sumOut/(inputsize * dt);
    cout << "Ibias:  " << Ibias*1E3 << "mA" << endl;
}
