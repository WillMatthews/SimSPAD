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
#include "ramlog.hpp"
#include <chrono>
#include <ctime>
#include <sstream>

#define COL_GREEN "\033[32;1m"
#define COL_RED "\033[31;1m"
#define COL_RESET "\033[0m"

// Provides the current time formatted as a string
std::string current_time(void)
{
  using namespace std;
  auto time = chrono::system_clock::now();
  time_t time_t = chrono::system_clock::to_time_t(time);
  string timestr = ctime(&time_t);
  return timestr;
}

long message_sequence = 0;
// Print a ostringstream to stdout, and also log to the circular buffer RamLog for access with /logs
// This function adds an [INFO] tag if no other tags are present. [ERROR] tags are red in stdout.
void message_print_log(std::ostringstream &message)
{
  message_sequence++;
  std::string msg = message.str();

  if (!(msg.contains("[INFO]") || msg.contains("[ERROR]") || msg.contains("[WARN]")))
  {
    msg = "[INFO]  " + msg;
  }

  std::string message_class = "";
  std::string COL_START = "";
  std::string COL_STOP = "";
  if (msg.contains("[ERROR]"))
  {
    message_class = " message-error";
    COL_START = COL_RED;
    COL_STOP = COL_RESET;
  }

  RamLog::getInstance().log("<tr><td class='sequence'>" + std::to_string(message_sequence) + "</td><td class='time'>" + current_time() + "</td><td class='message" + message_class + "'>" + msg + "</td></tr>");
  message.str("");
  std::cout << COL_START << msg << COL_STOP << std::endl;
}

// log web access events
void log_access(httplib::Request req, int status)
{
  std::string log_level = "[ERROR]";
  if (status < 0 || status == 200)
  {
    status = 200;
    log_level = "[INFO] ";
  }
  std::ostringstream message_buf;
  message_buf << log_level << " " << req.remote_addr << " has accessed '" << req.path << "' \t Status: " << status;
  message_print_log(message_buf);
}

int requests_served = 0;
long bytes_processed = 0;
std::string last_request_time = "None";

int main(void)
{
  cli_logo();
  std::cout << COL_GREEN << "\t   SimSPAD Server Running" << COL_RESET << std::endl;
  std::string start_time = current_time();
  std::cout << "Started at time: " << start_time << std::endl;

  using namespace httplib;

  // Manage SSL in nginx -> this server is intended to be used locally anyhow
  Server srv; // HTTP
  // SSLServer srv; // HTTPS

  // Limit Number of active threads - default to 12 if unset
  srv.new_task_queue = []
  { return new ThreadPool(4); };

  // Max payload size is 128 MB
  srv.set_payload_max_length(1024 * 1024 * 128);

  // If an error occurs (e.g. 404 -> redirect a user here)
  srv.set_error_handler([](const auto &req, auto &res)
                        {
    log_access(req, res.status);
    std::string page_text = page_header("Logs");
    page_text += "<p class='error'>Error Status: <span style='color:red;'>";
    page_text += std::to_string(res.status) + "</span></p>";
    page_text += page_footer();
    res.set_content(page_text, "text/html"); });

  // Landing page with basic instructions
  srv.Get("/", [](const Request &req, Response &res)
          {
    log_access(req, res.status);
    res.set_content(page_welcome(), "text/html"); });

  // Halt the server
  srv.Get("/stop", [&](const Request &req, Response &res)
          {
    std::string halttime = current_time();
    std::cout << COL_RED << "\t   Server halted via http" << COL_RESET << std::endl;
    std::cout << COL_RED << "\t     by: " << req.remote_addr << COL_RESET << std::endl;
    std::cout << "Halted at time: " << halttime << std::endl;
    res.set_content("Server Halted at " + halttime, "text/plain");
    srv.stop(); });

  // Present a user with logs of what the server has recently done
  srv.Get("/logs", [&](const Request &req, Response &res)
          {
    log_access(req, res.status);

    std::string page_text = page_header("Logs");
    page_text += "<div class='logs'>";

    auto row_lambda = [](std::string a, std::string b)
    { return "<tr><td class='info-name'>" + a + "</td><td>" + b + "</td></tr>"; };

    page_text += "<table>";
    page_text += row_lambda("Start Time:", start_time);
    page_text += row_lambda("Current Time:", current_time());
    page_text += row_lambda("Last Request At:", last_request_time);
    page_text += row_lambda("Requests Served:", std::to_string(requests_served));
    page_text += row_lambda("Bytes Processed:", std::to_string(bytes_processed));
    page_text += "</table><br/><br/>";

    std::string log_text = RamLog::getInstance().getLog();

    if (log_text.length() > 0)
    {
      page_text += "<table class='log-data'><tr><th>Seq.</th><th>Time</th><th>Data</th></tr>";
      page_text += log_text;
      page_text += "</table>";
    }

    page_text += "</div>" + page_footer();
    res.set_content(page_text, "text/html"); });

  // Run a Simulation from a POST request
  srv.Post("/simspad", [](const Request &req, Response &res)
           {
    last_request_time = current_time();
    std::ostringstream message_buf;
    using namespace std;
    message_buf << "==================== GOT POST ====================" << std::endl;
    message_print_log(message_buf);
    auto data = req.body;

    log_access(req, res.status);

    size_t numBytes = data.length();
    message_buf << "Decoding " << numBytes << " bytes..." << std::endl;
    bytes_processed += numBytes;
    message_print_log(message_buf);

    vector<double> optical_input = {};
    vector<double> sipmSettingsVector = {};
    unsigned char *bytes; // uchar* buffer for intermediate step converting char* to double
    double recv;          // Received double
    char buf[8];          // char buffer (incoming chars to be converted to floats)
    // Decode 8 chars to a double precision float
    for (size_t i = 0; i < numBytes / 8; i++)
    {
      for (int j = 0; j < 8; j++)
      {
        buf[j] = data[8 * i + j]; // Create buffer of char*
      }
      bytes = reinterpret_cast<unsigned char *>(&buf); // cast char* buffer to bytes
      recv = *reinterpret_cast<double *>(bytes);       // cast bytes to double

      if (i < 10) // First ten doubles are SiPM simulator parameters
      {
        sipmSettingsVector.push_back(recv);
      }
      else // Remainder of values are expected number of photons per dt striking array
      {
        optical_input.push_back(recv);
      }
    }

    // Create SiPM
    SiPM *sipm = new SiPM(sipmSettingsVector);

    // cout parameters so I can tell when someone does something stupid which breaks the server
    auto start = chrono::system_clock::now();
    time_t start_time = chrono::system_clock::to_time_t(start);
    message_buf << "Started computation at\t" << ctime(&start_time);
    message_print_log(message_buf);
    message_buf << "dt\t\t\t" << (sipm->dt) << " s";
    message_print_log(message_buf);
    message_buf << "NumMicrocells\t\t" << ((double)sipm->numMicrocell);
    message_print_log(message_buf);
    message_buf << "vBias\t\t\t" << (sipm->vBias) << " V";
    message_print_log(message_buf);
    message_buf << "vBreakdown\t\t" << (sipm->vBr) << " V";
    message_print_log(message_buf);
    message_buf << "TauRecovery\t\t" << (sipm->tauRecovery);
    message_print_log(message_buf);
    message_buf << "PDEMax\t\t\t" << (sipm->pdeMax);
    message_print_log(message_buf);
    message_buf << "vChrPDE\t\t\t" << (sipm->vChr) << " V";
    message_print_log(message_buf);
    message_buf << "CCell\t\t\t" << (sipm->cCell) << " F";
    message_print_log(message_buf);
    message_buf << "TauPulseFWHM\t\t" << (sipm->tauFwhm) << " s";
    message_print_log(message_buf);
    message_buf << "DigitalThreshold\t" << (sipm->digitalThreshold);
    message_print_log(message_buf);

    // Simulate
    bool silence = true;
    vector<double> response = sipm->simulate(optical_input, silence);

    // Print Elapsed Time (allow debugging)
    auto end = chrono::system_clock::now();
    chrono::duration<double> elapsed_seconds = end - start;
    message_buf << "Elapsed Time\t\t" << elapsed_seconds.count() * 1E3 << " ms";
    message_print_log(message_buf);

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

    // recycle buffer char buf[8] from earlier
    for (unsigned long i = 0; i < (unsigned long)sipm_output.size(); i++)
    {
      memcpy(&buf, &sipm_output[i], sizeof(buf));
      for (int j = 0; j < 8; j++)
      {
        outputString.push_back(buf[j]);
      }
    }
    numBytes = outputString.length();
    message_buf << "Sending Result (" << numBytes << " bytes)";
    message_print_log(message_buf);
    res.set_content(outputString, "text/plain");

    // Clean up - make 100% sure large variables are deleted
    delete sipm;
    // large vars:  bytes data recv optical_input response outputString sipm_output
    // These SHOULD be destroyed automatically when ending a transaction with the server. TODO CHECK

    requests_served++;
    message_buf << "==================== GOODBYE  ====================";
    message_print_log(message_buf); });

  srv.listen("127.0.0.1", 33232);
}
