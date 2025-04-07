#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// 1. 获取文件大小（如果不存在则返回 0）
std::uintmax_t get_file_size(const std::string& path) {
    std::error_code ec;
    auto sz = std::filesystem::file_size(path, ec);
    return ec ? 0 : sz;
}

// 2. 通过 socket 发送完整请求字符串
bool send_all(int sock, const std::string& data) {
    const char* ptr = data.data();
    std::size_t left = data.size();
    while (left > 0) {
        ssize_t sent = ::send(sock, ptr, left, 0);
        if (sent <= 0) return false;
        ptr += sent;
        left -= sent;
    }
    return true;
}

// 3. 简单读取 HTTP 响应，跳过头部，写 body 到文件
bool receive_and_save(int sock, std::ofstream& ofs) {
    constexpr size_t BUF_SZ = 8192;
    char buf[BUF_SZ];
    bool header_done = false;
    std::string header;

    while (true) {
        ssize_t n = ::recv(sock, buf, BUF_SZ, 0);
        if (n <= 0) break;

        if (!header_done) {
            header.append(buf, n);
            // 找到头体分隔："\r\n\r\n"
            auto pos = header.find("\r\n\r\n");
            if (pos != std::string::npos) {
                // body 从 pos+4 开始
                ofs.write(header.data() + pos + 4,
                          header.size() - (pos + 4));
                header_done = true;
            }
        } else {
            // 已经跳过头部，直接写入
            ofs.write(buf, n);
        }
    }
    return true;
}

bool download_with_resume(const std::string& host,
                          const std::string& port,
                          const std::string& target_path,
                          const std::string& output_path) {
    const std::string tmp_path = output_path + ".tmp";
    // —— 1. 计算偏移
    std::uintmax_t offset = get_file_size(tmp_path);
    std::cout << "Resuming from byte " << offset << "\n";

    // —— 2. DNS + 建立 TCP 连接
    addrinfo hints{}, *res;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &res) != 0)
        return false;

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) return false;
    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        close(sock);
        return false;
    }
    freeaddrinfo(res);

    // —— 3. 构造 HTTP GET 请求，带 Range 头
    std::ostringstream req;
    req << "GET " << target_path << " HTTP/1.1\r\n"
        << "Host: " << host << "\r\n"
        << "User-Agent: cpp-resume/1.0\r\n";
    if (offset > 0) {
        req << "Range: bytes=" << offset << "-\r\n";
    }
    req << "Connection: close\r\n\r\n";

    if (!send_all(sock, req.str())) {
        close(sock);
        return false;
    }

    // —— 4
