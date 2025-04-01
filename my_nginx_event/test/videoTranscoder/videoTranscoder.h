extern "C"{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <iostream>

class VideoTranscoder {
public:
    VideoTranscoder(const std::string& inputFile, const std::string& outputDir)
        : inputFile_(inputFile), outputDir_(outputDir) {
        avformat_network_init();
    }

    ~VideoTranscoder() {
        avformat_network_deinit();
    }

    bool transcodeToHLS() {
        AVFormatContext* inputFmtCtx = nullptr;
        AVCodecContext* decoderCtx = nullptr;
        AVFormatContext* outputFmtCtx = nullptr;
        AVCodecContext* encoderCtx = nullptr;

        // 1. 打开输入文件
        if (avformat_open_input(&inputFmtCtx, inputFile_.c_str(), nullptr, nullptr) < 0) {
            std::cerr << "Failed to open input file." << std::endl;
            return false;
        }
        avformat_find_stream_info(inputFmtCtx, nullptr);

        // 2. 找到视频流
        int videoStreamIndex = -1;
        for (unsigned int i = 0; i < inputFmtCtx->nb_streams; i++) {
            if (inputFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStreamIndex = i;
                break;
            }
        }
        if (videoStreamIndex == -1) {
            std::cerr << "No video stream found." << std::endl;
            return false;
        }

        // 3. 获取解码器
        AVCodec* decoder = avcodec_find_decoder(inputFmtCtx->streams[videoStreamIndex]->codecpar->codec_id);
        decoderCtx = avcodec_alloc_context3(decoder);
        avcodec_parameters_to_context(decoderCtx, inputFmtCtx->streams[videoStreamIndex]->codecpar);
        avcodec_open2(decoderCtx, decoder, nullptr);

        // 4. 初始化 H.264 编码器
        AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
        encoderCtx = avcodec_alloc_context3(encoder);
        encoderCtx->bit_rate = 400000;
        encoderCtx->width = decoderCtx->width;
        encoderCtx->height = decoderCtx->height;
        encoderCtx->time_base = {1, 25};
        encoderCtx->framerate = {25, 1};
        encoderCtx->gop_size = 10;
        encoderCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        encoderCtx->thread_count = 8;  // 启用多线程编码
        avcodec_open2(encoderCtx, encoder, nullptr);

        // 5. 创建 HLS 复用器
        avformat_alloc_output_context2(&outputFmtCtx, nullptr, "hls", (outputDir_ + "/output.m3u8").c_str());

        AVStream* outStream = avformat_new_stream(outputFmtCtx, encoder);
        avcodec_parameters_from_context(outStream->codecpar, encoderCtx);

        // HLS 相关配置
        av_dict_set(&outputFmtCtx->metadata, "hls_time", "5", 0);  // 每 5 秒一个 .ts
        av_dict_set(&outputFmtCtx->metadata, "hls_list_size", "0", 0);  // 保持所有 .ts 片段
        av_dict_set(&outputFmtCtx->metadata, "hls_flags", "delete_segments", 0);

        // 6. 打开输出文件
        if (!(outputFmtCtx->oformat->flags & AVFMT_NOFILE)) {
            avio_open(&outputFmtCtx->pb, (outputDir_ + "/output.m3u8").c_str(), AVIO_FLAG_WRITE);
        }
        avformat_write_header(outputFmtCtx, nullptr);

        // 7. 读取帧并转码
        AVPacket packet;
        while (av_read_frame(inputFmtCtx, &packet) >= 0) {
            if (packet.stream_index == videoStreamIndex) {
                avcodec_send_packet(decoderCtx, &packet);
                AVFrame* frame = av_frame_alloc();
                if (avcodec_receive_frame(decoderCtx, frame) == 0) {
                    avcodec_send_frame(encoderCtx, frame);
                    AVPacket encPacket;
                    av_init_packet(&encPacket);
                    encPacket.data = nullptr;
                    encPacket.size = 0;
                    if (avcodec_receive_packet(encoderCtx, &encPacket) == 0) {
                        av_interleaved_write_frame(outputFmtCtx, &encPacket);
                        av_packet_unref(&encPacket);
                    }
                    av_frame_free(&frame);
                }
            }
            av_packet_unref(&packet);
        }

        // 8. 结束处理
        av_write_trailer(outputFmtCtx);
        avio_closep(&outputFmtCtx->pb);
        avformat_free_context(outputFmtCtx);
        avcodec_free_context(&encoderCtx);
        avcodec_free_context(&decoderCtx);
        avformat_close_input(&inputFmtCtx);

        return true;
    }

private:
    std::string inputFile_;
    std::string outputDir_;
};