#include <sys/sendfile.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <string_view>
#include <cstring>

int main() {
    // 1. 创建并绑定监听 socket
    int listenfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        std::cerr << "socket error: " << std::strerror(errno) << "\n";
        return 1;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);
    if (::bind(listenfd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "bind error: " << std::strerror(errno) << "\n";
        return 1;
    }
    ::listen(listenfd, 1);

    std::cout << "Listening on port 8080...\n";

    // 2. 接受客户端连接
    int connfd = ::accept(listenfd, nullptr, nullptr);
    if (connfd < 0) {
        std::cerr << "accept error: " << std::strerror(errno) << "\n";
        return 1;
    }
    std::cout << "Client connected, sending file...\n";

    // 3. 打开要发送的文件
    const char* filename = "testfile.bin";
    int filefd = ::open(filename, O_RDONLY);
    if (filefd < 0) {
        std::cerr << "open file error: " << std::strerror(errno) << "\n";
        ::close(connfd);
        return 1;
    }

    // 4. 获取文件大小
    off_t offset = 0;
    const auto fileSize = ::lseek(filefd, 0, SEEK_END);
    ::lseek(filefd, 0, SEEK_SET);

    // 5. 通过 sendfile 在内核空间完成文件到 socket 的拷贝
    ssize_t bytesSent = ::sendfile(connfd, filefd, &offset, static_cast<size_t>(fileSize));
    if (bytesSent < 0) {
        std::cerr << "sendfile error: " << std::strerror(errno) << "\n";
    } else {
        std::cout << "Sent " << bytesSent << " bytes from " << filename << "\n";
    }

    // 6. 清理资源
    ::close(filefd);
    ::close(connfd);
    ::close(listenfd);
    return 0;
}
