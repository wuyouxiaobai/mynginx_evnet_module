extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}
#include <unordered_map>
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
        AVFormatContext* ifmt_ctx = nullptr;
        AVFormatContext* ofmt_ctx = nullptr;
        std::unordered_map<int, AVCodecContext*> dec_ctx_map;
        std::unordered_map<int, AVCodecContext*> enc_ctx_map;
        std::unordered_map<int, SwrContext*> swr_ctx_map;

        if (avformat_open_input(&ifmt_ctx, inputFile_.c_str(), nullptr, nullptr) < 0 ||
            avformat_find_stream_info(ifmt_ctx, nullptr) < 0) {
            std::cerr << "Could not open input file.\n";
            return false;
        }

        avformat_alloc_output_context2(&ofmt_ctx, nullptr, "hls", outputDir_.c_str());
        if (!ofmt_ctx) {
            std::cerr << "Could not create output context.\n";
            return false;
        }

        std::unordered_map<int, int64_t> pts_counter;

        for (unsigned int i = 0; i < ifmt_ctx->nb_streams; i++) {
            AVStream* in_stream = ifmt_ctx->streams[i];
            AVCodecParameters* in_codecpar = in_stream->codecpar;
            AVCodec* decoder = avcodec_find_decoder(in_codecpar->codec_id);
            AVCodecContext* dec_ctx = avcodec_alloc_context3(decoder);
            avcodec_parameters_to_context(dec_ctx, in_codecpar);
            avcodec_open2(dec_ctx, decoder, nullptr);
            dec_ctx_map[i] = dec_ctx;

            AVStream* out_stream = avformat_new_stream(ofmt_ctx, nullptr);
            pts_counter[i] = 0;

            if (in_codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                AVCodec* encoder = avcodec_find_encoder_by_name("libx264");
                AVCodecContext* enc_ctx = avcodec_alloc_context3(encoder);
                enc_ctx->height = dec_ctx->height;
                enc_ctx->width = dec_ctx->width;
                enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
                enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

                AVRational input_fps = av_guess_frame_rate(ifmt_ctx, in_stream, nullptr);
                enc_ctx->time_base = av_inv_q(input_fps);
                enc_ctx->framerate = input_fps;
                enc_ctx->bit_rate = 400000;
                enc_ctx->gop_size = 25;
                enc_ctx->max_b_frames = 0;
                enc_ctx->thread_count = 4;
                enc_ctx->thread_type = FF_THREAD_SLICE;
                av_opt_set(enc_ctx->priv_data, "preset", "veryfast", 0);
                if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                    enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
                avcodec_open2(enc_ctx, encoder, nullptr);
                avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
                enc_ctx_map[i] = enc_ctx;
            } else if (in_codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
                AVCodecContext* enc_ctx = avcodec_alloc_context3(encoder);
                enc_ctx->sample_rate = dec_ctx->sample_rate;
                enc_ctx->channel_layout = dec_ctx->channel_layout;
                enc_ctx->channels = dec_ctx->channels;
                enc_ctx->sample_fmt = encoder->sample_fmts[0];
                enc_ctx->bit_rate = 128000;
                avcodec_open2(enc_ctx, encoder, nullptr);
                avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
                enc_ctx_map[i] = enc_ctx;

                SwrContext* swr_ctx = swr_alloc_set_opts(nullptr,
                    enc_ctx->channel_layout, enc_ctx->sample_fmt, enc_ctx->sample_rate,
                    dec_ctx->channel_layout, dec_ctx->sample_fmt, dec_ctx->sample_rate,
                    0, nullptr);
                swr_init(swr_ctx);
                swr_ctx_map[i] = swr_ctx;
            }
        }

        AVDictionary* opts = nullptr;
        av_dict_set(&opts, "hls_time", "5", 0);
        av_dict_set(&opts, "hls_list_size", "0", 0);
        av_dict_set(&opts, "hls_flags", "delete_segments+append_list", 0);

        if (avio_open(&ofmt_ctx->pb, outputDir_.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open output file.\n";
            return false;
        }

        if (avformat_write_header(ofmt_ctx, &opts) < 0) {
            std::cerr << "Error writing header.\n";
            return false;
        }

        AVPacket* pkt = av_packet_alloc();
        AVFrame* frame = av_frame_alloc();
        AVPacket* enc_pkt = av_packet_alloc();

        while (av_read_frame(ifmt_ctx, pkt) >= 0) {
            int stream_idx = pkt->stream_index;
            if (dec_ctx_map.find(stream_idx) == dec_ctx_map.end()) {
                av_packet_unref(pkt);
                continue;
            }

            AVCodecContext* dec_ctx = dec_ctx_map[stream_idx];
            AVCodecContext* enc_ctx = enc_ctx_map[stream_idx];
            AVStream* out_stream = ofmt_ctx->streams[stream_idx];

            if (avcodec_send_packet(dec_ctx, pkt) >= 0) {
                while (avcodec_receive_frame(dec_ctx, frame) == 0) {
                    frame->pts = pts_counter[stream_idx]++;

                    if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO && swr_ctx_map.count(stream_idx)) {
                        AVFrame* resampled = av_frame_alloc();
                        resampled->channel_layout = enc_ctx->channel_layout;
                        resampled->format = enc_ctx->sample_fmt;
                        resampled->sample_rate = enc_ctx->sample_rate;
                        resampled->nb_samples = frame->nb_samples;
                        av_frame_get_buffer(resampled, 0);

                        swr_convert(swr_ctx_map[stream_idx], resampled->data, resampled->nb_samples,
                                    (const uint8_t**)frame->data, frame->nb_samples);
                        resampled->pts = frame->pts;
                        frame = resampled;
                    }

                    if (avcodec_send_frame(enc_ctx, frame) >= 0) {
                        while (avcodec_receive_packet(enc_ctx, enc_pkt) == 0) {
                            av_packet_rescale_ts(enc_pkt, enc_ctx->time_base, out_stream->time_base);
                            enc_pkt->stream_index = stream_idx;
                            av_interleaved_write_frame(ofmt_ctx, enc_pkt);
                            av_packet_unref(enc_pkt);
                        }
                    }
                    av_frame_unref(frame);
                }
            }
            av_packet_unref(pkt);
        }

        av_write_trailer(ofmt_ctx);

        for (auto& [idx, ctx] : dec_ctx_map) avcodec_free_context(&ctx);
        for (auto& [idx, ctx] : enc_ctx_map) avcodec_free_context(&ctx);
        for (auto& [idx, swr] : swr_ctx_map) swr_free(&swr);
        avformat_close_input(&ifmt_ctx);
        if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
            avio_closep(&ofmt_ctx->pb);
        avformat_free_context(ofmt_ctx);
        av_frame_free(&frame);
        av_packet_free(&pkt);
        av_packet_free(&enc_pkt);
        return true;
    }

private:
    std::string inputFile_;
    std::string outputDir_;
};
    