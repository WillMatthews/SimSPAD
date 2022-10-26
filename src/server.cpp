#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../lib/cpp-httplib/httplib.h"

int main(void)
{
  using namespace httplib;
  // HTTP
  Server svr;

  // HTTPS
  // SSLServer svr;

  svr.Get("/hi", [](const Request &, Response &res)
          { res.set_content("Hello World!", "text/plain"); });

  svr.Get(R"(/numbers/(\d+))", [&](const Request &req, Response &res)
          {
    auto numbers = req.matches[1];
    res.set_content(numbers, "text/plain"); });

  svr.Get("/stop", [&](const Request &req, Response &res)
          {
            (void) req;
            (void) res;
            svr.stop(); });

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

svr.Post("/content_receiver",
         [&](const Request &req, Response &res, const ContentReader &content_reader)
         {
           if (req.is_multipart_form_data())
           {
             // NOTE: `content_reader` is blocking until every form data field is read
             MultipartFormDataItems files;
             content_reader(
                 [&](const MultipartFormData &file)
                 {
                   files.push_back(file);
                   return true;
                 },
                 [&](const char *data, size_t data_length)
                 {
                   files.back().content.append(data, data_length);
                   return true;
                 });
           }
           else
           {
             std::string body;
             content_reader([&](const char *data, size_t data_length)
                            {
        body.append(data, data_length);
        return true; });
           }
         });

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