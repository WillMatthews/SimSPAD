//#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../lib/cpp-httplib/httplib.h"
#include "sipm.hpp"
#include <chrono>
#include <ctime>

std::string welcome()
{
  std::string welcomestring = "";
  welcomestring += "Welcome to SimSPAD";
  welcomestring += "\n";
  welcomestring += "Access the GitHub Repository for more information:\n";
  welcomestring += "https://github.com/WillMatthews/SimSPAD\n";

  return welcomestring;
}

int main(void)
{
  std::cout << "Server Running" << std::endl;

  using namespace httplib;
  // HTTP
  Server svr;

  // HTTPS
  // SSLServer svr;

  // Limit Number of active threads - default 12
  svr.new_task_queue = []
  { return new ThreadPool(4); };

  svr.set_payload_max_length(1024 * 1024 * 512);

  svr.Get("/", [](const Request &, Response &res)
          { res.set_content(welcome(), "text/plain"); });

  svr.Get("/stop", [&](const Request &req, Response &res)
          {
            (void) req;
            (void) res;
            svr.stop(); });

  svr.Post("/simspad", [](const Request &req, Response &res)
           {
             using namespace std;
             cout << "=====================================" << endl;

             // cout << req.body << endl;
             auto data = req.body;

             size_t data_length = data.length();
             size_t numbits = data_length;

             unsigned char *bytes;
             double recv;
             char buf[8];

             vector<double> optical_input = {};
             vector<double> sipmvars = {};

             std::cout << "Decoding " << numbits << " bytes..." << endl;
             for (size_t i = 0; i < numbits / 8; i++)
             {
               for (int j = 0; j < 8; j++)
               {
                 buf[j] = data[8 * i + j];
               }
               bytes = reinterpret_cast<unsigned char *>(&buf);
               recv = *reinterpret_cast<double *>(bytes);

               if (i < 10)
               {
                 sipmvars.push_back(recv);
               }
               else
               {
                 optical_input.push_back(recv);
               }
               // std::cout << recv << std::endl;
             }

             vector<double> response = {};
             bool silence = true;
             SiPM sipm(sipmvars);

             auto start = chrono::system_clock::now();
             time_t start_time = chrono::system_clock::to_time_t(start);
             cout << "Started computation at " << ctime(&start_time) << endl;
             // cout output so I can tell when someone does something stupid which breaks the server
             cout << "dt " << (sipm.dt) << endl;
             cout << "nuc " << ((double)sipm.numMicrocell) << endl;
             cout << "vb " << (sipm.vBias) << endl;
             cout << "vbr " << (sipm.vBr) << endl;
             cout << "tau " << (sipm.tauRecovery) << endl;
             cout << "mpde " << (sipm.pdeMax) << endl;
             cout << "vchr " << (sipm.vChr) << endl;
             cout << "ccell " << (sipm.cCell) << endl;
             cout << "tau " << (sipm.tauFwhm) << endl;
             cout << "digithresh " << (sipm.digitalThreshhold) << endl;

             //simulate
             response = sipm.simulate(optical_input, silence);

             auto end = chrono::system_clock::now();
             chrono::duration<double> elapsed_seconds = end-start;
             cout << "elapsed time: " << elapsed_seconds.count() << "s" << std::endl;

             vector<double> sipm_output = {};
             sipm_output.push_back(sipm.dt);
             sipm_output.push_back((double)sipm.numMicrocell);
             sipm_output.push_back(sipm.vBias);
             sipm_output.push_back(sipm.vBr);
             sipm_output.push_back(sipm.tauRecovery);
             sipm_output.push_back(sipm.pdeMax);
             sipm_output.push_back(sipm.vChr);
             sipm_output.push_back(sipm.cCell);
             sipm_output.push_back(sipm.tauFwhm);
             sipm_output.push_back(sipm.digitalThreshhold);

             sipm_output.insert(sipm_output.end(), response.begin(), response.end());

             string outputstring = "";

             char c[8];
             for (int i = 0; i < (int)sipm_output.size(); i++)
             {

               memcpy(&c, &sipm_output[i], sizeof(c));

               outputstring.push_back(c[0]);
               outputstring.push_back(c[1]);
               outputstring.push_back(c[2]);
               outputstring.push_back(c[3]);
               outputstring.push_back(c[4]);
               outputstring.push_back(c[5]);
               outputstring.push_back(c[6]);
               outputstring.push_back(c[7]);
             }

             res.set_content(outputstring, "text/plain"); });

  svr.listen("localhost", 33232);
}
