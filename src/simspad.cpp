
#include <iostream>
#include <vector>
#include <string>
#include <iterator>
#include <cmath>
#include <random>
#include <chrono>
#include <thread>
#include <time.h>
#include "sipm.hpp"
#include "sipmcsv.hpp"

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



// create lambda expression for a simulation run (input csv -> output csv).
// might be helpful if multithreading in the future
auto sim_lambda = [](string fname){
    vector<double> out = {};
    //vector<double> in = {};
    double dt;
    
    int ber_run_samples = 844759;
    vector<double> in(ber_run_samples,10); // DC light source
    dt = 1E-10;

    SiPM j30020(14410, 27.5, 24.5, 2.2*14E-9, 0.0, 4.6e-14, 2.04, 0.46);
    //tie(in,dt) = readCSV(fname+".csv");
    j30020.dt = dt;

    auto start = chrono::steady_clock::now();

    out = j30020.simulate(in);
    //out = j30020.simulate_full(in);
    //j30020.test_rand_funcs();

    auto end = chrono::steady_clock::now();
    chrono::duration<double> elapsed = end-start;

    string prefix;
    double val;
    tie(prefix, val) = exponent_val(elapsed.count());
    cout << "\nElapsed Time:\t " << val << prefix << "s" <<  endl;
    tie(prefix, val) = exponent_val(dt*in.size());
    cout << "Simulated Time:\t " << val << prefix << "s" << endl;
    tie(prefix, val) = exponent_val(dt);
    cout << "Resolution:\t " << val << prefix << "s" << endl;
    cout << "Time Steps:\t " << in.size() << "Sa" <<  endl;
    double time_per_iter = ( (double) elapsed.count()) / ( (double) in.size());
    tie(prefix, val) = exponent_val(time_per_iter);
    cout << "Computer Time Per Step:\t " << val << prefix << "s" << endl;
    tie(prefix, val) = exponent_val(time_per_iter/j30020.numMicrocell);
    cout << "Computer Time Per uCell Step:\t " << val << prefix  << "s" << endl;

    double sumOut = 0;
    for (int i = 0; i< in.size(); i++){
        sumOut += out[i];
    }
    double Ibias = sumOut/(in.size() * j30020.dt);
    cout << "Ibias:  " << Ibias*1E3 << "mA" << endl;

    writeCSV(fname+"_sim_out.csv", out, j30020);
};


// Run simulation and time
int main(int argc, char *argv[]){

    unsigned int time_ui = static_cast<unsigned int>( time(NULL) );
    srand(time_ui);

    ios::sync_with_stdio(0);
    cin.tie(0);

    sim_lambda("siminput");
    
    return EXIT_SUCCESS;
}
