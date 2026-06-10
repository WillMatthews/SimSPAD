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
 *    EXPERIMENTAL CODE. Input is now validated and there are basic CSRF / DNS-rebinding
 *    defenses (Host allowlist + Origin check on state-changing endpoints) plus an optional
 *    API key (SIMSPAD_API_KEY); see the README. It is still intended to run locally behind a
 *    trusted proxy, NOT exposed directly to the open web. I assume NO LIABILITY if you run
 *    this code. If you proceed, you acknowledge you understand the risks involved.
 */

// #define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../lib/cpp-httplib/httplib.h"
#include "sipm.hpp"
#include "utilities.hpp"
#include "pages.hpp"
#include "ramlog.hpp"
#include <chrono>
#include <ctime>
#include <sstream>
#include <memory>
#include <vector>
#include <map>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <algorithm>

#define COL_GREEN "\033[32;1m"  // Used for server messages
#define COL_YELLOW "\033[33;1m" // Used for Warnings
#define COL_RED "\033[31;1m"    // Used for Errors
#define COL_RESET "\033[0m"     // Reset colour in term

// Provides the current time formatted as a string
std::string current_time(void)
{
  using namespace std;
  auto time = chrono::system_clock::now();
  time_t time_t = chrono::system_clock::to_time_t(time);
  string timestr = ctime(&time_t);
  return timestr;
}

long message_sequence = 0;      // message sequence number (how many messages have been logged)
// Print a ostringstream to stdout, and also log to the circular buffer RamLog for access with /logs
// This function adds an [INFO] tag if no other tags are present. [ERROR] tags are red in stdout.
// [WARN] tags are yellow (orange) in stdout.
// The logging to the circular buffer RamLog logs preformatted HTML table rows, with appropriate
// CSS classes for the quoted log level.
void message_print_log(std::ostringstream &message)
{
  message_sequence++;
  std::string msg = message.str();

  // if no tag is defined - give the message an INFO tag.
  if (!((msg.find("[INFO]") != std::string::npos) || (msg.find("[ERROR]") != std::string::npos) || (msg.find("[WARN]") != std::string::npos)))
  {
    msg = "[INFO]  " + msg;
  }

  std::string message_class = ""; // CSS class (see pages.cpp) for message
  std::string COL_START = "";   // ANSI code for stdout
  std::string COL_STOP  = "";   // ANSI code for stdout
  if (msg.find("[ERROR]") != std::string::npos)
  {
    // if message has an Error tag, give it the appropriate tags
    message_class = "message-error";
    COL_START = COL_RED;
    COL_STOP = COL_RESET;
  }
  else if (msg.find("[WARN]") != std::string::npos)
  {
    // if message has an Warning tag, give it the appropriate tags
    message_class = "message-warn";
    COL_START = COL_YELLOW;
    COL_STOP = COL_RESET;
  }
  else
  {
    // Otherwise, give the info tag
    message_class = "message-info";
  }

  RamLog::getInstance().log("<tr><td class='sequence'>" + std::to_string(message_sequence) + "</td><td class='time'>" + current_time() + "</td><td class='message " + message_class + "'>" + msg + "</td></tr>");
  message.str("");
  std::cout << COL_START << msg << COL_STOP << std::endl;
}

// HTML-escape a string so attacker-controlled, request-derived data (e.g. req.path,
// req.remote_addr) cannot inject markup when the log buffer is rendered as text/html at /logs.
// Guards against stored XSS (GHSA-mvgv-c4rv-99ch).
std::string html_escape(const std::string &in)
{
  std::string out;
  out.reserve(in.size());
  for (char c : in)
  {
    switch (c)
    {
    case '&':
      out += "&amp;";
      break;
    case '<':
      out += "&lt;";
      break;
    case '>':
      out += "&gt;";
      break;
    case '"':
      out += "&quot;";
      break;
    case '\'':
      out += "&#x27;";
      break;
    default:
      out += c;
      break;
    }
  }
  return out;
}

// log web access events (IP addr and request path) as an [INFO] event
// Test status code of the response to a request. If an error status code is thrown, log as an error.
void log_access(httplib::Request req, int status)
{
  std::string log_level = "[WARN] ";
  if (status < 0 || status == 200)
  {
    status = 200;
    log_level = "[INFO] ";
  }
  std::ostringstream message_buf;
  // req.remote_addr and req.path are attacker-controlled and end up in an HTML page (/logs);
  // escape them before they reach the RamLog buffer.
  message_buf << log_level << " " << html_escape(req.remote_addr) << " has accessed '" << html_escape(req.path) << "' \t Status: " << status;
  message_print_log(message_buf);
}

int requests_served = 0;    // number of simulation requests served
long bytes_processed = 0;   // number of bytes processed by server
std::string last_request_time = "None"; // When the last request happened

// Per-request DoS caps for POST /simspad. Simulation cost is O(N * numMicrocell), both
// taken from the unauthenticated request; numMicrocell is bounded by MAX_MICROCELL in the
// SiPM constructor, and these bound the waveform length and the total work product so a
// crafted request cannot exhaust CPU/RAM (GHSA-f2ph-wv99-c83q). Tune to taste; the
// canonical 5676-cell device at MAX_SAMPLES (~9.1e10) stays under MAX_WORK, so realistic
// runs are unaffected and only pathological numMicrocell*N combinations are rejected.
constexpr size_t MAX_SAMPLES = 16000000UL;          // ~122 MB of float64 input
constexpr uint64_t MAX_WORK = 100000000000ULL;      // 1e11 microcell-steps per request

// ---- CSRF / DNS-rebinding defenses (GHSA-x9fq-39h6-5x58) ----
// The server binds loopback, but a browser the operator is using can still be coerced into
// issuing requests to it (CSRF), and DNS-rebinding can make a foreign page's requests resolve
// to 127.0.0.1. We defend with a Host allowlist (rebinding: the rebound attacker hostname
// won't match), an Origin check on state-changing endpoints (cross-site CSRF), and an optional
// API key. Loopback defaults work out of the box; override behind a reverse proxy.

// Split a comma-separated list, trimming surrounding whitespace from each element.
static std::vector<std::string> split_csv(const std::string &s)
{
  std::vector<std::string> out;
  size_t i = 0;
  while (i <= s.size())
  {
    size_t comma = s.find(',', i);
    if (comma == std::string::npos)
      comma = s.size();
    size_t a = i, b = comma;
    while (a < b && std::isspace((unsigned char)s[a]))
      ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1]))
      --b;
    if (b > a)
      out.push_back(s.substr(a, b - a));
    i = comma + 1;
  }
  return out;
}

// Allowed Host header values (host[:port]). Override with SIMSPAD_ALLOWED_HOSTS
// (comma-separated) when running behind a reverse proxy that forwards a public Host.
static const std::vector<std::string> &allowed_hosts()
{
  static const std::vector<std::string> hosts = []
  {
    const char *env = std::getenv("SIMSPAD_ALLOWED_HOSTS");
    return split_csv((env && *env) ? env
                                   : "127.0.0.1:33232,localhost:33232,127.0.0.1,localhost");
  }();
  return hosts;
}

static bool host_allowed(const std::string &host)
{
  const auto &h = allowed_hosts();
  return std::find(h.begin(), h.end(), host) != h.end();
}

// Origin is acceptable if absent (non-browser clients such as curl/requests never send it) or
// its host[:port] is allowlisted. A cross-site browser request carries a foreign Origin.
static bool origin_allowed(const std::string &origin)
{
  if (origin.empty())
    return true;
  size_t pos = origin.find("://");
  std::string hostport = (pos == std::string::npos) ? origin : origin.substr(pos + 3);
  return host_allowed(hostport);
}

// Authorise a state-changing request (/stop, /simspad). Host is already enforced globally by
// the pre-routing handler; here we reject cross-site Origins and, if SIMSPAD_API_KEY is set,
// require a matching X-API-Key. Returns true to proceed; otherwise sets res and returns false.
static bool state_change_authorised(const httplib::Request &req, httplib::Response &res)
{
  if (!origin_allowed(req.get_header_value("Origin")))
  {
    res.status = 403;
    res.set_content("forbidden: cross-origin request rejected", "text/plain");
    return false;
  }
  const char *key = std::getenv("SIMSPAD_API_KEY");
  if (key && *key && req.get_header_value("X-API-Key") != key)
  {
    res.status = 401;
    res.set_content("unauthorized: missing or invalid X-API-Key", "text/plain");
    return false;
  }
  return true;
}

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

  // Defense-in-depth against XSS: a restrictive Content-Security-Policy on every response
  // so any markup that slips into a rendered page cannot execute script (GHSA-mvgv-c4rv-99ch).
  // img-src allows 'self' (favicon) and data: (the embedded base64 SVG logo).
  srv.set_post_routing_handler([](const auto & /*req*/, auto &res)
                               { res.set_header("Content-Security-Policy",
                                                "default-src 'none'; style-src 'unsafe-inline'; img-src 'self' data:; base-uri 'none'; form-action 'none'"); });

  // Reject any request whose Host header is not allowlisted, before routing. This defeats
  // DNS-rebinding (a rebound attacker hostname won't match the loopback allowlist) across
  // every endpoint, including the read-only /logs (GHSA-x9fq-39h6-5x58).
  srv.set_pre_routing_handler([](const Request &req, Response &res) -> Server::HandlerResponse
                              {
    if (!host_allowed(req.get_header_value("Host")))
    {
      res.status = 403;
      res.set_content("forbidden: Host not allowlisted", "text/plain");
      return Server::HandlerResponse::Handled;
    }
    return Server::HandlerResponse::Unhandled; });

  // srv.set_logger([](const auto &req, const auto &res)
  //                { log_access(req, res.status); });

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

  // Halt the server. POST, not GET: a destructive state change must not be reachable by a
  // simple <img src> / link prefetch, and is additionally gated by Origin/API-key checks
  // (GHSA-x9fq-39h6-5x58).
  srv.Post("/stop", [&](const Request &req, Response &res)
          {
    if (!state_change_authorised(req, res))
    {
      log_access(req, res.status);
      return;
    }
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

    // define a lambda function for a HTML table row
    auto row_lambda = [](std::string a, std::string b)
    { return "<tr><td class='info-name'>" + a + "</td><td>" + b + "</td></tr>"; };

    page_text += "<table>";
    page_text += row_lambda("Start Time:", start_time);
    page_text += row_lambda("Current Time:", current_time());
    page_text += row_lambda("Last Request At:", last_request_time);
    page_text += row_lambda("Requests Served:", std::to_string(requests_served));
    page_text += row_lambda("Bytes Processed:", std::to_string(bytes_processed));
    page_text += "</table><br/><br/>";

    // Print the log table if the table length is greater than zero.
    std::string log_text = RamLog::getInstance().getLog();
    if (log_text.length() > 0)
    {
      page_text += "<table class='log-data'><tr><th>Seq.</th><th>Time</th><th>Data</th></tr>";
      page_text += log_text;
      page_text += "</table>";
    }

    page_text += "</div>" + page_footer();
    res.set_content(page_text, "text/html"); });

  // Run a Simulation from a POST request.
  //
  // Protocol: device parameters arrive as a flat JSON object in the
  // `X-SiPM-Params` request header; the request body is the raw optical-input
  // waveform -- little-endian float64, expected photons-per-dt striking the
  // whole array, one value per time step. The response is streamed back as
  // `application/octet-stream`: little-endian float64 charge-per-step, the same
  // length as the input, produced in bounded-memory chunks off the streaming
  // core (no giant output buffer, no positional parameter header).
  srv.Post("/simspad", [](const Request &req, Response &res)
           {
    last_request_time = current_time();
    std::ostringstream message_buf;
    using namespace std;
    message_buf << "==================== GOT POST ====================" << std::endl;
    message_print_log(message_buf);

    log_access(req, res.status);

    // Reject cross-site / unauthorised requests before doing any work (GHSA-x9fq-39h6-5x58).
    if (!state_change_authorised(req, res))
    {
      message_buf << "[ERROR] rejected request: failed Origin/API-key check (status "
                  << res.status << ")";
      message_print_log(message_buf);
      return;
    }

    // --- device parameters (JSON header) ---
    string paramJson = req.get_header_value("X-SiPM-Params");
    if (paramJson.empty())
    {
      res.status = 400;
      res.set_content("missing X-SiPM-Params header (JSON device parameters)", "text/plain");
      message_buf << "[ERROR] rejected request: no X-SiPM-Params header";
      message_print_log(message_buf);
      return;
    }

    map<string, double> pm = parse_flat_json(paramJson);
    static const char *keys[10] = {"dt", "numMicrocell", "vBias", "vBr", "tauRecovery",
                                   "pdeMax", "vChr", "cCell", "tauFwhm", "digitalThreshold"};
    vector<double> svars(10);
    for (int i = 0; i < 10; i++)
    {
      auto kv = pm.find(keys[i]);
      if (kv == pm.end())
      {
        res.status = 400;
        res.set_content(string("X-SiPM-Params missing key: ") + keys[i], "text/plain");
        message_buf << "[ERROR] rejected request: params missing key " << keys[i];
        message_print_log(message_buf);
        return;
      }
      svars[i] = kv->second;
    }

    // --- optical-input waveform (octet-stream body) ---
    const string &body = req.body;
    if (body.size() % sizeof(double) != 0)
    {
      res.status = 400;
      res.set_content("body length is not a multiple of 8 (expect a float64 waveform)", "text/plain");
      message_buf << "[ERROR] rejected request: body length " << body.size() << " not a multiple of 8";
      message_print_log(message_buf);
      return;
    }
    size_t N = body.size() / sizeof(double);

    // Bound per-request work before allocating or simulating (GHSA-f2ph-wv99-c83q).
    // Cap the waveform length first so we never copy an oversized body.
    if (N > MAX_SAMPLES)
    {
      res.status = 413;
      res.set_content("input waveform too long (max " + std::to_string(MAX_SAMPLES) + " samples)", "text/plain");
      message_buf << "[ERROR] rejected request: " << N << " samples exceeds MAX_SAMPLES " << MAX_SAMPLES;
      message_print_log(message_buf);
      return;
    }
    bytes_processed += (long)body.size();

    // Build the device first. Invalid/out-of-range parameters throw from the SiPM
    // constructor (length, finiteness, numMicrocell range) and become a clean 400
    // here rather than an uncaught 500.
    std::shared_ptr<SiPM> sipm;
    try
    {
      sipm = make_shared<SiPM>(svars);
    }
    catch (const std::invalid_argument &e)
    {
      res.status = 400;
      res.set_content(std::string("invalid device parameters: ") + e.what(), "text/plain");
      message_buf << "[ERROR] rejected request: " << e.what();
      message_print_log(message_buf);
      return;
    }

    // Work-product budget. numMicrocell is already capped to MAX_MICROCELL, so this
    // only rejects pathological numMicrocell*N combinations (e.g. a tiny body with a
    // huge cell count) without affecting realistic simulations.
    if ((uint64_t)sipm->numMicrocell * (uint64_t)N > MAX_WORK)
    {
      res.status = 413;
      res.set_content("simulation too large (numMicrocell * samples exceeds the per-request budget)", "text/plain");
      message_buf << "[ERROR] rejected request: work "
                  << ((uint64_t)sipm->numMicrocell * (uint64_t)N) << " exceeds MAX_WORK " << MAX_WORK;
      message_print_log(message_buf);
      return;
    }

    // The chunked provider below outlives this handler call, so copy the input
    // out of the (soon-to-be-destroyed) request.
    auto input = make_shared<vector<double>>(N);
    memcpy(input->data(), body.data(), body.size());

    // Seed the initial age distribution from the raw mean light level (matching
    // the in-memory simulate()).
    double rawSum = 0.0;
    for (size_t i = 0; i < N; i++)
    {
      rawSum += (*input)[i];
    }
    sipm->init_state(N ? rawSum / (double)N : 0.0, (unsigned long)N);

    message_buf << "Streaming " << N << " samples (" << body.size() << " bytes in)";
    message_print_log(message_buf);

    // Stream the response in bounded-memory chunks straight off the streaming
    // core. The provider runs after this handler returns, hence the shared_ptr
    // captures (sipm/input outlive the request; pos carries progress).
    auto pos = make_shared<size_t>(0);
    res.set_chunked_content_provider(
        "application/octet-stream",
        [sipm, input, pos, N](size_t /*offset*/, httplib::DataSink &sink) -> bool
        {
          const size_t chunk = 1u << 16; // 65536 samples per block
          size_t n = (N - *pos < chunk) ? (N - *pos) : chunk;
          if (n > 0)
          {
            vector<double> out(n);
            sipm->simulate_chunk(input->data() + *pos, out.data(), n);
            if (!sink.write(reinterpret_cast<const char *>(out.data()), n * sizeof(double)))
            {
              return false; // client went away
            }
            *pos += n;
          }
          if (*pos >= N)
          {
            sink.done();
          }
          return true;
        });

    requests_served++;
    message_buf << "==================== GOODBYE  ====================";
    message_print_log(message_buf); });

  srv.listen("127.0.0.1", 33232);
}
