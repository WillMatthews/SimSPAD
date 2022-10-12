
#include <iostream>
#include <vector>
#include <string>
#include <iterator>
#include <cmath>
#include <random>
#include <chrono>
#include <thread>
#include <time.h>
#include "sipmcsv.hpp"

using namespace std;

// create lambda expression for a simulation run (input csv -> output csv).
// might be helpful if multithreading in the future
auto sim_lambda = [](string fname){
    vector<double> out = {};
    vector<double> in(200000,10) ;;
    double dt = 1E-10;

    SiPM j30020(14410, 27.5, 24.5, 2.2*14E-9, 0.0, 4.6e-14);
    //tie(in,dt) = readCSV(fname+".csv");
    j30020.dt = dt;

    auto start = chrono::steady_clock::now();

    out = j30020.simulate(in);

    auto end = chrono::steady_clock::now();
    chrono::duration<double> elapsed = end-start;

    cout << "\nElapsed Time:\t " << elapsed.count() << "s" <<  endl;
    cout << "Simulated Time:\t " << dt*in.size() << "s" << endl;
    cout << "Resolution:\t " << dt << "s" << endl;

    /*
    for (int i = 0; i< in.size(); i++){
        cout << in[i] << "\t\t" << out[i] << endl;
    }
    */

    writeCSV(fname+"_out.csv", out, j30020);
};

// Run simulation and time
int main(int argc, char *argv[]){

    unsigned int time_ui = static_cast<unsigned int>( time(NULL) );
    srand(time_ui);

    ios::sync_with_stdio(0);
    cin.tie(0);

    sim_lambda("siminput");
    
    /*
    // Need a better way of parallelising - every function O(n), but all are memory intensivve -- am I limited by memory bandwidth?
    vector<thread> some_threads;
    for (int i=0; i<1; i++){
        some_threads.push_back(thread(sim_lambda,"siminput"));
    }
    for (auto& a: some_threads){
        a.join();
    }
    */


    return EXIT_SUCCESS;
}
