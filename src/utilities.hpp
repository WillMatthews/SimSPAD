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
#include <string>
#include <cmath>

using namespace std;

tuple<string, double> exponent_val(double num){
    int floor_prefix = floor(log10(num));
    string prefixes[9] = {"f", "p", "n", "u", "m", "", "k", "M", "G"};
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
        return make_tuple("", num);
    }
    double pot = (double) -(idx_prefix * 3 - 15);
    return make_tuple(prefixes[idx_prefix], num*pow(10,pot)); 
}


void print_info(chrono::duration<double> elapsed, double dt, vector<double>outvec, int numMicrocell){
    auto sysclock = chrono::system_clock::now();
    auto tt = chrono::system_clock::to_time_t(sysclock);
    string timeStr = ctime(&tt);

    int inputsize = outvec.size();

    cout << "\n\nSimulation Run at: " << timeStr << endl;

    string prefix;
    double val;
    tie(prefix, val) = exponent_val(elapsed.count());
    cout << "Elapsed Time:\t " << val << prefix << "s" <<  endl;
    tie(prefix, val) = exponent_val(dt*inputsize);
    cout << "Simulated Time:\t " << val << prefix << "s" << endl;
    tie(prefix, val) = exponent_val(dt);
    cout << "Resolution:\t " << val << prefix << "s" << endl;
    cout << "Time Steps:\t " << inputsize << "Sa" <<  endl;
    double time_per_iter = ( (double) elapsed.count()) / ( (double) inputsize);
    tie(prefix, val) = exponent_val(time_per_iter);
    cout << "Computer Time Per Step:\t " << val << prefix << "s" << endl;
    tie(prefix, val) = exponent_val(time_per_iter/numMicrocell);
    cout << "Computer Time Per uCell Step:\t " << val << prefix  << "s" << endl;

    double sumOut = 0;
    for (int i = 0; i< inputsize; i++){
        sumOut += outvec[i];
    }
    double Ibias = sumOut/(inputsize * dt);
    cout << "Ibias:  " << Ibias*1E3 << "mA" << endl;
}
