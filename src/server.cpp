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

// #define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../lib/cpp-httplib/httplib.h"
#include "sipm.hpp"
#include "pages.hpp"
#include <chrono>
#include <ctime>

#define GREEN "\033[32;1m"
#define RED "\033[31;1m"
#define COL_RESET "\033[0m"

int main(void)
{
    cli_logo();
    std::cout << GREEN << "\t   SimSPAD Server Running" << COL_RESET << std::endl;

    using namespace httplib;

    Server srv; // HTTP
    // SSLServer srv; // HTTPS

    // Limit Number of active threads - default 12
    srv.new_task_queue = []
    { return new ThreadPool(4); };

    // Max payload size is 128 MB
    srv.set_payload_max_length(1024 * 1024 * 128);

    srv.Get("/", [](const Request &, Response &res)
            { res.set_content(welcome(), "text/html"); });

    srv.Get("/stop", [&](const Request &req, Response &res)
            {
            using namespace std;
            (void) req;
            auto halt_time = chrono::system_clock::now();
            time_t halt_time_t = chrono::system_clock::to_time_t(halt_time);
            string halttime = ctime(&halt_time_t);
            cout << RED << "Halted from web interface" << COL_RESET << endl;
            cout << "At time "<< halttime << endl;
            res.set_content("Server halted at " + halttime, "text/plain");
            srv.stop(); });

    srv.Post("/simspad", [](const Request &req, Response &res)
             {
             using namespace std;
             cout << "==================== GOT POST ====================" << endl;
             auto data = req.body;

             size_t numBits = data.length();
             std::cout << "Decoding " << numBits << " bytes..." << endl;

             vector<double> optical_input = {};
             vector<double> sipmSettingsVector = {};
             unsigned char *bytes;  // uchar* buffer for intermediate step converting char* to double
             double recv;           // Received double
             char buf[8];           // char buffer (incoming chars to be converted to floats)
             // Decode 8 chars to a double precision float
             for (size_t i = 0; i < numBits / 8; i++)
             {
               for (int j = 0; j < 8; j++)
               {
                 buf[j] = data[8 * i + j]; // Create buffer of char*
               }
               bytes = reinterpret_cast<unsigned char *>(&buf); // cast char* buffer to bytes
               recv = *reinterpret_cast<double *>(bytes);       // cast bytes to double

               if (i < 10)  // First ten doubles are SiPM simulator parameters
               {
                 sipmSettingsVector.push_back(recv);
               }
               else         // Remainder of values are expected number of photons per dt striking array
               {
                 optical_input.push_back(recv);
               }
             }

             // Create SiPM
             SiPM *sipm = new SiPM(sipmSettingsVector);

             // cout parameters so I can tell when someone does something stupid which breaks the server
             auto start = chrono::system_clock::now();
             time_t start_time = chrono::system_clock::to_time_t(start);
             cout << "Started computation at\t" << ctime(&start_time) << endl;
             cout << "dt\t\t\t" << (sipm->dt) << " s" << endl;
             cout << "NumMicrocells\t\t" << ((double)sipm->numMicrocell) << endl;
             cout << "vBias\t\t\t" << (sipm->vBias) << " V" << endl;
             cout << "vBreakdown\t\t" << (sipm->vBr) << " V" << endl;
             cout << "TauRecovery\t\t" << (sipm->tauRecovery) << " s" << endl;
             cout << "PDEMax\t\t\t" << (sipm->pdeMax) << endl;
             cout << "vChrPDE\t\t\t" << (sipm->vChr) << " V" << endl;
             cout << "CCell\t\t\t" << (sipm->cCell) << " F" << endl;
             cout << "TauPulseFWHM\t\t" << (sipm->tauFwhm) << " s" << endl;
             cout << "DigitalThreshold\t" << (sipm->digitalThreshold) << endl;

             // Simulate
             bool silence = true;
             vector<double> response = sipm->simulate(optical_input, silence);

             // Print Elapsed Time (allow debugging)
             auto end = chrono::system_clock::now();
             chrono::duration<double> elapsed_seconds = end-start;
             cout << "Elapsed Time:\t\t" << elapsed_seconds.count()*1E3 << " ms" << std::endl;

             // create output vector
             vector<double> sipm_output = {};
             sipm_output.push_back(sipm->dt);
             sipm_output.push_back((double)sipm->numMicrocell);
             sipm_output.push_back(sipm->vBias);
             sipm_output.push_back(sipm->vBr);
             sipm_output.push_back(sipm->tauRecovery);
             sipm_output.push_back(sipm->pdeMax);
             sipm_output.push_back(sipm->vChr);
             sipm_output.push_back(sipm->cCell);
             sipm_output.push_back(sipm->tauFwhm);
             sipm_output.push_back(sipm->digitalThreshold);

             // concat SiPM simulation output on end of input parameters
             sipm_output.insert(sipm_output.end(), response.begin(), response.end());

             // create output string. char* (in blocks of 8 for each double) are appended
             // for the output via the web response
             string outputString = "";

             //recycle buffer char buf[8] from earlier
             for (unsigned long i = 0; i < (unsigned long)sipm_output.size(); i++)
             {
               memcpy(&buf, &sipm_output[i], sizeof(buf));
               for (int j = 0; j < 8; j++){
                outputString.push_back(buf[j]);
               }
             }

             cout << "Sending Result" << endl;
             res.set_content(outputString, "text/plain");

             // Clean up - make 100% sure large variables are deleted
             delete sipm;
             // large vars:  bytes data recv optical_input response outputString sipm_output

             cout << "==================== GOODBYE  ====================" << endl; });

    srv.listen("127.0.0.1", 33232);
}
