#include <iostream>
#include <thread>

#include <boost/asio.hpp>
#include <opencv2/opencv.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include "frame_queue.h"

using boost::asio::ip::udp;

int main() {
    // -----------------------
    // OpenCV webcam
    // -----------------------
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Failed to open webcam\n";
        return -1;
    }

    int width = 500, height = 480;
    cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    cap.set(cv::CAP_PROP_BUFFERSIZE, 1);

    FrameQueue frame_queue(5);

    // -----------------------
    // FFmpeg H.264 encoder
    // -----------------------
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "H.264 encoder not found\n";
        return -1;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    codec_ctx->width = width;
    codec_ctx->height = height;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx->time_base = {1, 30};
    codec_ctx->framerate = {30,1};
    codec_ctx->gop_size = 1;
    codec_ctx->max_b_frames = 0;
    codec_ctx->bit_rate = 800000;

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        std::cerr << "Could not open codec\n";
        return -1;
    }

    AVFrame* frame = av_frame_alloc();
    frame->format = codec_ctx->pix_fmt;
    frame->width = codec_ctx->width;
    frame->height = codec_ctx->height;
    av_frame_get_buffer(frame, 32);

    AVPacket* pkt = av_packet_alloc();

    SwsContext* sws_ctx = sws_getContext(
        width, height, AV_PIX_FMT_BGR24,
        width, height, AV_PIX_FMT_YUV420P,
        SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
        );

    // -----------------------
    // Boost.Asio UDP socket
    // -----------------------
    boost::asio::io_context io;
    udp::socket socket(io);
    socket.open(udp::v4());
    //udp::endpoint target(boost::asio::ip::make_address("127.0.0.1"), 5000);
    udp::endpoint target(boost::asio::ip::make_address("192.168.1.131"), 5000);

    // -----------------------
    // Capture thread
    // -----------------------
    std::thread capture_thread([&]{
        while (true) {
            cv::Mat bgr;
            cap >> bgr;
            if (bgr.empty()) break;

            frame_queue.push(bgr);

            if (cv::waitKey(1) == 27) break; // exit on ESC
        }
    });

    // -----------------------
    // Encode + send thread
    // -----------------------
    std::thread encode_thread([&]{
        int frame_index = 0;
        while (true) {
            cv::Mat bgr = frame_queue.pop();
            if (bgr.empty()) continue;

            uint8_t* in_data[1] = { bgr.data };
            int in_linesize[1] = { static_cast<int>(bgr.step) };

            sws_scale(
                sws_ctx,
                in_data,
                in_linesize,
                0,
                height,
                frame->data,
                frame->linesize
                );

            frame->pts = frame_index++;

            if (avcodec_send_frame(codec_ctx, frame) == 0) {
                while (avcodec_receive_packet(codec_ctx, pkt) == 0) {
                    socket.send_to(boost::asio::buffer(pkt->data, pkt->size), target);
                    av_packet_unref(pkt);
                }
            }
        }
    });

    capture_thread.join();
    encode_thread.join();

    // -----------------------
    // Cleanup
    // -----------------------
    sws_freeContext(sws_ctx);
    av_packet_free(&pkt);
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);

    return 0;
}
