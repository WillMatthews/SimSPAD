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

/*
 *  I8,        8        ,8I    db         88888888ba   888b      88  88  888b      88    ,ad8888ba,
 *  `8b       d8b       d8'   d88b        88      "8b  8888b     88  88  8888b     88   d8"'    `"8b
 *   "8,     ,8"8,     ,8"   d8'`8b       88      ,8P  88 `8b    88  88  88 `8b    88  d8'
 *    Y8     8P Y8     8P   d8'  `8b      88aaaaaa8P'  88  `8b   88  88  88  `8b   88  88
 *    `8b   d8' `8b   d8'  d8YaaaaY8b     88""""88'    88   `8b  88  88  88   `8b  88  88      88888
 *     `8a a8'   `8a a8'  d8""""""""8b    88    `8b    88    `8b 88  88  88    `8b 88  Y8,        88
 *      `8a8'     `8a8'  d8'        `8b   88     `8b   88     `8888  88  88     `8888   Y8a.    .a88
 *       `8'       `8'  d8'          `8b  88      `8b  88      `888  88  88      `888    `"Y88888P"
 *
 *                                             READ ME!!
 *
 *    VERY EXPERIMENTAL CODE: NO INPUT SANITATION (yet), NO API KEY AUTH (yet), VERY UNSAFE CODE.
 *    DO NOT EXPOSE TO OUTSIDE WEB. It's probably a poor idea to run this code on your own server.
 *    This code needs a significant amount of effort to make it safe. I assume NO LIABILITY if you
 *    run this code. If you proceed, you acknowledge you understand the risks involved.
 */

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
  std::cout << "SimSPAD Server Running" << std::endl;

  using namespace httplib;

  Server srv; // HTTP
  // SSLServer srv; // HTTPS

  // Limit Number of active threads - default 12
  srv.new_task_queue = []
  { return new ThreadPool(4); };

  // max payload size is 512 MB
  srv.set_payload_max_length(1024 * 1024 * 512);

  srv.Get("/", [](const Request &, Response &res)
          { res.set_content(welcome(), "text/plain"); });

  srv.Get("/stop", [&](const Request &req, Response &res)
          {
            (void) req;
            (void) res;
            srv.stop(); });

  srv.Post("/simspad", [](const Request &req, Response &res)
           {
             using namespace std;
             cout << "==================== GOT POST ====================" << endl;
             auto data = req.body;

             size_t numbits = data.length();
             std::cout << "Decoding " << numbits << " bytes..." << endl;

             vector<double> optical_input = {};
             vector<double> sipmvars = {};
             unsigned char *bytes; // uchar* buffer for intermediate step converting char* to double
             double recv; // received double
             char buf[8]; // char buffer (incoming chars to be converted to floats)
             // decode 8 chars to a double precision float
             for (size_t i = 0; i < numbits / 8; i++)
             {
               for (int j = 0; j < 8; j++)
               {
                 buf[j] = data[8 * i + j]; // create buffer of char*
               }
               bytes = reinterpret_cast<unsigned char *>(&buf); // cast char* buffer to bytes
               recv = *reinterpret_cast<double *>(bytes); // cast bytes to double

               if (i < 10) // first ten doubles are SiPM simulator parameters
               {
                 sipmvars.push_back(recv);
               }
               else // remainder of values are expected number of photons per dt striking array
               {
                 optical_input.push_back(recv);
               }
             }

             // create SiPM
             SiPM sipm(sipmvars);

             // cout parameters so I can tell when someone does something stupid which breaks the server
             auto start = chrono::system_clock::now();
             time_t start_time = chrono::system_clock::to_time_t(start);
             cout << "Started computation at " << ctime(&start_time) << endl;
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

             // simulate
             vector<double> response = {};
             bool silence = true;
             response = sipm.simulate(optical_input, silence);

             // Print Elapsed Time (allow debugging)
             auto end = chrono::system_clock::now();
             chrono::duration<double> elapsed_seconds = end-start;
             cout << "elapsed time: " << elapsed_seconds.count() << "s" << std::endl;

             // create output vector
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

             // concat SiPM simulation output on end of input parameters
             sipm_output.insert(sipm_output.end(), response.begin(), response.end());

             // create output string. char* (in blocks of 8 for each double) are appended
             // for the output via the web response
             string outputstring = "";

             //recycle buffer char buf[8] from earlier
             for (int i = 0; i < (int)sipm_output.size(); i++)
             {
               memcpy(&buf, &sipm_output[i], sizeof(buf));
               for (int j = 0; j < 8; j++){
                outputstring.push_back(buf[j]);
               }
             }
             cout << "==================== GOODBYE  ====================" << endl;

             res.set_content(outputstring, "text/plain"); });

  srv.listen("localhost", 33232);
}