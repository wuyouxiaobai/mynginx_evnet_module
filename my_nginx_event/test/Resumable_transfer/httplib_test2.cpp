#include "httplib.h"
#include <fstream>
#include <filesystem>

using namespace httplib;
namespace fs = std::filesystem;

const std::string FILE_DIR = "./files";  // 文件目录

std::string get_mime_type(const std::string &ext) {
    if (ext == ".mp4") return "video/mp4";
    if (ext == ".txt") return "text/plain";
    if (ext == ".pdf") return "application/pdf";
    return "application/octet-stream";
}

int main() {
    Server svr;

    svr.Get(R"(/(.+))", [](const Request& req, Response& res) {
        std::string filename = req.matches[1];
        std::string filepath = FILE_DIR + "/" + filename;

        if (!fs::exists(filepath)) {
            res.status = 404;
            res.set_content("File not found.", "text/plain");
            return;
        }

        std::ifstream ifs(filepath, std::ios::binary);
        if (!ifs.is_open()) {
            res.status = 500;
            res.set_content("Unable to open file.", "text/plain");
            return;
        }

        std::uintmax_t file_size = fs::file_size(filepath);
        std::uintmax_t range_start = 0;
        std::uintmax_t range_end = file_size - 1;

        // 解析 Range: bytes=x-y
        auto it = req.headers.find("Range");
        if (it != req.headers.end()) {
            std::string range_value = it->second;
            if (sscanf(range_value.c_str(), "bytes=%llu-%llu", &range_start, &range_end) >= 1) {
                if (range_end >= file_size) range_end = file_size - 1;
                if (range_start > range_end) range_start = 0;
                res.status = 206;
                res.set_header("Content-Range", 
                    "bytes " + std::to_string(range_start) + "-" + 
                    std::to_string(range_end) + "/" + std::to_string(file_size));
            }
        } else {
            res.status = 200;
        }

        std::uintmax_t send_len = range_end - range_start + 1;
        std::string buffer(send_len, 0);
        ifs.seekg(range_start);
        ifs.read(&buffer[0], send_len);

        std::string ext = fs::path(filepath).extension().string();
        res.set_content(std::move(buffer), get_mime_type(ext));
        res.set_header("Accept-Ranges", "bytes");
        res.set_header("Content-Length", std::to_string(send_len));
    });

    std::cout << "Server running on http://localhost:8080/\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}
