
#include <thread>
#define NUM_THREADS 4

using namespace std;

class SiPM {
    #define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
    #define PBWIDTH 60
    public:
        int numMicrocell = 14410;
        double vbias = 27.5;
        double vbr = 24.5;
        double vover = vbias-vbr;
        double dt; // get from input
        double tauRecovery = 2.2*14E-9;
        double digitalThreshhold = 0;
        double ccell = 1;

        inline double pde_fcn(double overvoltage){
            return 0.46*(1-exp(-(overvoltage/vbr)/0.083));
        }

        inline double pde_from_time(double time){
            double v = volt_from_time(time);
            return pde_fcn(v);
        }

        inline double volt_from_time(double time){
            return vover * (1-exp(-time/tauRecovery));
        }



        int getNumMicrocell(void){
            return numMicrocell;
        }

        vector<double> simulate(vector<double> light){

            vector<double> qFired;
            double l;
            double pctdone;

            init_spads();
            for (int i=0; i<light.size(); i++){
                /*if (i%100 == 0){
                    pctdone = (double)i/(double)light.size();
                    print_progress(pctdone);
                }*/
                l = light[i];
                qFired.push_back(recharge_illuminate(l));
            }

            return qFired;
        }

    static const size_t LUTSize = 15;

    double tVecLUT[LUTSize]   = { 0 };
    double pdeVecLUT[LUTSize] = { 0 };
    double vVecLUT[LUTSize]   = { 0 };

    void initLUT(void){
        int numpoint = (int) LUTSize;
        double maxt = 5.3*tauRecovery;
        double ddt  = (double)maxt/numpoint;
        for (int i=0; i<numpoint; i++){
            tVecLUT[i] = i*ddt;
            vVecLUT[i] = vover * (1-exp(-tVecLUT[i]/tauRecovery));
            pdeVecLUT[i] = pde_fcn(vVecLUT[i]);
        }
    }

    //private:
    //
        vector<double> microcellTimes=vector<double>(numMicrocell, 0.0);
        vector<double> microcellVoltages=vector<double>(numMicrocell, 0.0); 

        double randab(double a, double b){
            return (a + static_cast <double> (rand()) / ( static_cast <double> (RAND_MAX/(b-a))));
        }

        void init_spads(void){
            for (int i=0; i<numMicrocell; i++){
                microcellTimes[i] = tauRecovery*randab(0,10);
            }
        }

        double recharge_illuminate(double photonsPerSecond){
            double output = 0;
            double volt = 0;
            for (int i=0; i<numMicrocell; i++){
                microcellTimes[i] += dt;
                if (randab(0,1) < (pdeLUT(microcellTimes[i])*(photonsPerSecond/numMicrocell))){
                    volt = voltLUT(microcellTimes[i]);
                    microcellTimes[i] = 0;
                    if (volt > digitalThreshhold * vover){
                        output += volt*ccell;
                    }
                }
            }
            return output;
        }

    void print_progress(double percentage) {
        int val = (int) (percentage * 100);
        int lpad = (int) (percentage * PBWIDTH);
        int rpad = PBWIDTH - lpad;
        printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
        fflush(stdout);
    }


    double pdeLUT(double x){
        double* xs = tVecLUT;
        double* ys = pdeVecLUT;
        // number of elements in the array 
        const int count = (const int) LUTSize;

        int i;
        double dx, dy;

        if (x < xs[0]) {
            //x is less than the minimum element
            // handle error here if you want 
            return ys[0]; // return minimum element 
        }

        if (x > xs[count-1]) {
            return ys[count-1]; // return maximum 
        }

        // find i, such that xs[i] <= x < xs[i+1] 
        for (i = 0; i < count-1; i++) {
            if (xs[i+1] > x) {
                break;
            }
        }

        // interpolate 
        dx = xs[i+1] - xs[i];
        dy = ys[i+1] - ys[i];
        return ys[i] + (x - xs[i]) * dy / dx;
    }

    double voltLUT(double x){
        double* xs = tVecLUT;
        double* ys = vVecLUT;

        // number of elements in the array 
        const int count = (const int) LUTSize;

        int i;
        double dx, dy;

        if (x < xs[0]) {
            //x is less than the minimum element
            // handle error here if you want 
            return ys[0]; // return minimum element 
        }

        if (x > xs[count-1]) {
            return ys[count-1]; // return maximum 
        }

        // find i, such that xs[i] <= x < xs[i+1] 
        for (i = 0; i < count-1; i++) {
            if (xs[i+1] > x) {
                break;
            }
        }

        // interpolate 
        dx = xs[i+1] - xs[i];
        dy = ys[i+1] - ys[i];
        return ys[i] + (x - xs[i]) * dy / dx;
    }

};

