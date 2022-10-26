//#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../lib/cpp-httplib/httplib.h"
#include "sipm.hpp"

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

  svr.set_payload_max_length(1024 * 1024 * 512);

  svr.Get("/", [](const Request &, Response &res)
          { res.set_content(welcome(), "text/plain"); });

  svr.Get("/stop", [&](const Request &req, Response &res)
          {
            (void) req;
            (void) res;
            svr.stop(); });

  /*  svr.Post("/rx",
             [&](const Request &req, Response &res, const ContentReader &content_reader)
             {
               std::cout << "GOT POST:" << std::endl;


               if (req.is_multipart_form_data())
               {
                 // NOTE: `content_reader` is blocking until every form data field is read
                 MultipartFormDataItems files;
                 content_reader(
                     [&](const MultipartFormData &file)
                     {
                       files.push_back(file);
                       res.set_content("OK1", "text/plain");
                       return true;
                     },
                     [&](const char *data, size_t data_length)
                     {
                       files.back().content.append(data, data_length);
                       res.set_content("OK2", "text/plain");
                       return true;
                     });
               }
               else
               {
                 (void)req;
                 std::string body;
                 content_reader([&](const char *data, size_t data_length)
                                {
                 body.append(data, data_length);
                 size_t numbits = data_length/8;

                 unsigned char* bytes;
                 double recv;
                 char buf[8];

                 std::cout << "Decoding " << numbits << " bits...";
                 for (size_t i = 0; i<numbits; i++){
                  for (int j = 0; j<8; j++){
                    buf[j] = data[8*i+j];
                  }
                  bytes = reinterpret_cast<unsigned char*>(&buf);
                  recv = *reinterpret_cast<double*>(bytes);
                  //std::cout << recv << std::endl;
                  (void) recv;
                 }
                 std::cout << " Done" << std::endl;

                 res.set_content("OK3", "text/plain");
                 return true; });
               }
               // res.set_content("OK", "text/plain");
             });
  */

  svr.Post("/rx", [](const Request &req, Response &res)
           {
    using namespace std;

    //cout << req.body << endl;
    auto data = req.body;

    size_t data_length = data.length();
    size_t numbits = data_length;

    unsigned char* bytes;
    double recv;
    char buf[8];

    vector<double> optical_input = {};
    vector<double> sipmvars = {};

    std::cout << "Decoding " << numbits << " bytes..." << endl;
    for (size_t i = 0; i<numbits/8; i++){
      for (int j = 0; j<8; j++){
        buf[j] = data[8*i+j];
      }
      bytes = reinterpret_cast<unsigned char*>(&buf);
      recv = *reinterpret_cast<double*>(bytes);

      if (i<10){
        sipmvars.push_back(recv);
      } else {
        optical_input.push_back(recv);
      }
      //std::cout << recv << std::endl;
    }

    vector<double> response = {};
    bool silence = false;
    SiPM sipm(sipmvars);
    response = sipm.simulate(optical_input, silence);


    /*
    double value;
    char* vals[8];
    for (int i = 0; i< (int) sipm_out.size(); i++){
      value = sipm_out[i];
      vals = reinterpret_cast<char*>(&value);
      cout << value << " " << vals  << "X" << endl;
      strcat(result, (const char*) vals);
    }
    */

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

    sipm_output.insert(sipm_output.end(), response.begin(), response.end());

    string outputstring = "";
    //char buf_out[8];
    //unsigned char* bytes_out;

    //bytes_out = reinterpret_cast<unsigned char*>(&buf_out);
    //sim_output[i] = *reinterpret_cast<double*>(bytes_out);

    //union tconv {
    //  double d;
    //  char c[8];
    //};

    char c[8];
    for (int i = 0; i < (int) sipm_output.size(); i++){
      //unsigned char * bytes_out = reinterpret_cast<unsigned char *>(&sipm_output[i]);

      //double d;
      //memcpy( &d, array, sizeof( d ) );

      memcpy(&c, &sipm_output[i], sizeof(c));
      cout << c << endl;

      outputstring.push_back(c[0]);
      outputstring.push_back(c[1]);
      outputstring.push_back(c[2]);
      outputstring.push_back(c[3]);
      outputstring.push_back(c[4]);
      outputstring.push_back(c[5]);
      outputstring.push_back(c[6]);
      outputstring.push_back(c[7]);
    }
    cout << "Bytes in Result: " << sizeof(sipm_output[0])*sipm_output.size() << endl;
    cout << "Bytes in Output: " << sizeof(outputstring[0])*outputstring.size() << endl;




    res.set_content(outputstring, "text/plain"); });

  svr.listen("localhost", 33232);
}

/*
svr.Get("/body-header-param", [](const Request &req, Response &res)
        {
    if (req.has_header("Content-Length")) {
      auto val = req.get_header_value("Content-Length");
    }
    if (req.has_param("key")) {
      auto val = req.get_param_value("key");
    }
    res.set_content(req.body, "text/plain"); });

const size_t DATA_CHUNK_SIZE = 4;

svr.Get("/stream", [&](const Request &req, Response &res)
        {
  auto data = new std::string("abcdefg");

  res.set_content_provider(
    data->size(), // Content length
    "text/plain", // Content type
    [data](size_t offset, size_t length, DataSink &sink) {
      const auto &d = *data;
      sink.write(&d[offset], std::min(length, DATA_CHUNK_SIZE));
      return true; // return 'false' if you want to cancel the process.
    },
    [data](bool success) { delete data; }); });
*/