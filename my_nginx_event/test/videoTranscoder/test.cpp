#include "videoTranscoder.h"

int main() {
    VideoTranscoder transcoder("input.mp4", "./hls_output");
    if (transcoder.transcodeToHLS()) {
        std::cout << "HLS 转码完成！\n";
    } else {
        std::cerr << "HLS 转码失败！\n";
    }
    return 0;
}