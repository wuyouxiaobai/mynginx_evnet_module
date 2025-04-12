#include <exception>
#include <string>
#include "my_http_response.h"


namespace WYXB{



class HttpException : public std::exception {
public:
    HttpException(HttpResponse::HttpStatusCode code, const std::string& msg)
        : code_(code), msg_(msg) {}

    const char* what() const noexcept override { return msg_.c_str(); }
    HttpResponse::HttpStatusCode code() const noexcept { return code_; }

private:
    HttpResponse::HttpStatusCode code_;
    std::string msg_;
};

}