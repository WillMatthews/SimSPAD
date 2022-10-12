
#include <iostream>
#include <vector>
#include <string>
#include <iterator>
#include <cmath>
#include <random>
#include <chrono>
#include <time.h>
#include "sipmcsv.hpp"



using namespace std;

// create lambda expression for a simulation.
// might be helpful if multithreading in the future
auto sim_lambda = [](string fname){
    vector<double> out;
    vector<double> in;
    double dt;

    SiPM j30020;
    
    tie(in,dt) = readCSV(fname+".csv");
    j30020.dt = dt;
    j30020.initLUT();

    out=j30020.simulate(in);

    writeCSV(fname+"_out.csv", out, j30020);
};

// Run simulation and time
int main(int argc, char *argv[]){

    unsigned int time_ui = static_cast<unsigned int>( time(NULL) );
    srand( time_ui );

    ios::sync_with_stdio(0);
    cin.tie(0);

    auto start = std::chrono::steady_clock::now();
    //simulate
    //sim_lambda("siminput");
    vector<thread> some_threads;
    for (int i=0; i<1; i++){
        some_threads.push_back(thread(sim_lambda,"siminput"));
    }
    for (auto& a: some_threads){
        a.join();
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end-start;

    cout << "\nElapsed Time: " << elapsed.count() << "s" <<  endl;
    return EXIT_SUCCESS;
}
