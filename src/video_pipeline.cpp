#include "video_pipeline.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#if FOCUS_VIDEO_HAS_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
}
#endif

namespace focus_video {
namespace {

#if FOCUS_VIDEO_HAS_FFMPEG
std::string ffmpeg_error(int code) {
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(code, buffer, sizeof(buffer));
    return buffer;
}

void throw_if_negative(int code, const std::string& action) {
    if (code < 0) {
        throw std::runtime_error(action + ": " + ffmpeg_error(code));
    }
}

VideoStreamInfo stream_info(const AVStream* stream, const AVCodecParameters* params) {
    VideoStreamInfo info;
    info.width = static_cast<std::uint32_t>(std::max(params->width, 0));
    info.height = static_cast<std::uint32_t>(std::max(params->height, 0));
    const AVRational rate = stream->avg_frame_rate.num != 0 ? stream->avg_frame_rate : stream->r_frame_rate;
    if (rate.num > 0 && rate.den > 0) {
        info.fps = av_q2d(rate);
    }
    if (const AVCodecDescriptor* descriptor = avcodec_descriptor_get(params->codec_id)) {
        info.codec_name = descriptor->name;
    } else {
        info.codec_name = "unknown";
    }
    info.pixel_format = av_get_pix_fmt_name(static_cast<AVPixelFormat>(params->format)) != nullptr
                            ? av_get_pix_fmt_name(static_cast<AVPixelFormat>(params->format))
                            : "unknown";
    return info;
}
#endif

std::string yes_no(bool value) {
    return value ? "yes" : "no";
}

}  // namespace

bool ffmpeg_backend_compiled() {
#if FOCUS_VIDEO_HAS_FFMPEG
    return true;
#else
    return false;
#endif
}

VideoProbeResult probe_video(const std::filesystem::path& path) {
#if FOCUS_VIDEO_HAS_FFMPEG
    AVFormatContext* raw_context = nullptr;
    throw_if_negative(avformat_open_input(&raw_context, path.string().c_str(), nullptr, nullptr), "open input");
    struct ContextGuard {
        AVFormatContext* context = nullptr;
        ~ContextGuard() { avformat_close_input(&context); }
    } guard{raw_context};

    throw_if_negative(avformat_find_stream_info(raw_context, nullptr), "find stream info");
    const int video_index = av_find_best_stream(raw_context, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_index < 0) {
        throw std::runtime_error("No video stream found in: " + path.string());
    }

    VideoProbeResult result;
    result.path = path;
    if (raw_context->duration > 0) {
        result.duration_seconds = static_cast<double>(raw_context->duration) / AV_TIME_BASE;
    }
    const AVStream* video_stream = raw_context->streams[video_index];
    result.video = stream_info(video_stream, video_stream->codecpar);
    result.has_audio = av_find_best_stream(raw_context, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0) >= 0;
    return result;
#else
    (void)path;
    throw std::runtime_error("Focus Video was built without in-process FFmpeg demux/decode support.");
#endif
}

DecodeSmokeResult decode_video_smoke(const std::filesystem::path& path, std::uint32_t max_frames) {
#if FOCUS_VIDEO_HAS_FFMPEG
    if (max_frames == 0) {
        throw std::runtime_error("decode smoke max_frames must be greater than zero.");
    }

    AVFormatContext* raw_context = nullptr;
    throw_if_negative(avformat_open_input(&raw_context, path.string().c_str(), nullptr, nullptr), "open input");
    struct ContextGuard {
        AVFormatContext* context = nullptr;
        ~ContextGuard() { avformat_close_input(&context); }
    } context_guard{raw_context};

    throw_if_negative(avformat_find_stream_info(raw_context, nullptr), "find stream info");
    const int video_index = av_find_best_stream(raw_context, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_index < 0) {
        throw std::runtime_error("No video stream found in: " + path.string());
    }

    AVStream* video_stream = raw_context->streams[video_index];
    const AVCodec* codec = avcodec_find_decoder(video_stream->codecpar->codec_id);
    if (codec == nullptr) {
        throw std::runtime_error("No decoder available for codec: " + std::string(avcodec_get_name(video_stream->codecpar->codec_id)));
    }

    AVCodecContext* raw_codec_context = avcodec_alloc_context3(codec);
    if (raw_codec_context == nullptr) {
        throw std::runtime_error("Unable to allocate FFmpeg codec context.");
    }
    struct CodecGuard {
        AVCodecContext* context = nullptr;
        ~CodecGuard() { avcodec_free_context(&context); }
    } codec_guard{raw_codec_context};

    throw_if_negative(avcodec_parameters_to_context(raw_codec_context, video_stream->codecpar), "copy codec parameters");
    throw_if_negative(avcodec_open2(raw_codec_context, codec, nullptr), "open decoder");

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    if (packet == nullptr || frame == nullptr) {
        av_packet_free(&packet);
        av_frame_free(&frame);
        throw std::runtime_error("Unable to allocate FFmpeg packet/frame.");
    }
    struct PacketFrameGuard {
        AVPacket* packet = nullptr;
        AVFrame* frame = nullptr;
        ~PacketFrameGuard() {
            av_packet_free(&packet);
            av_frame_free(&frame);
        }
    } packet_frame_guard{packet, frame};

    DecodeSmokeResult result;
    result.probe.path = path;
    if (raw_context->duration > 0) {
        result.probe.duration_seconds = static_cast<double>(raw_context->duration) / AV_TIME_BASE;
    }
    result.probe.video = stream_info(video_stream, video_stream->codecpar);
    result.probe.has_audio = av_find_best_stream(raw_context, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0) >= 0;

    auto drain_frames = [&]() {
        while (result.decoded_frames < max_frames) {
            const int receive = avcodec_receive_frame(raw_codec_context, frame);
            if (receive == AVERROR(EAGAIN) || receive == AVERROR_EOF) {
                break;
            }
            throw_if_negative(receive, "receive decoded frame");
            ++result.decoded_frames;
            if (result.first_frame_format.empty()) {
                const char* format = av_get_pix_fmt_name(static_cast<AVPixelFormat>(frame->format));
                result.first_frame_format = format != nullptr ? format : "unknown";
            }
            av_frame_unref(frame);
        }
    };

    while (result.decoded_frames < max_frames) {
        const int read = av_read_frame(raw_context, packet);
        if (read == AVERROR_EOF) {
            throw_if_negative(avcodec_send_packet(raw_codec_context, nullptr), "flush decoder");
            drain_frames();
            break;
        }
        throw_if_negative(read, "read packet");
        if (packet->stream_index == video_index) {
            throw_if_negative(avcodec_send_packet(raw_codec_context, packet), "send packet");
            drain_frames();
        }
        av_packet_unref(packet);
    }

    if (result.decoded_frames == 0) {
        throw std::runtime_error("FFmpeg demux succeeded but no video frames were decoded.");
    }
    return result;
#else
    (void)path;
    (void)max_frames;
    throw std::runtime_error("Focus Video was built without in-process FFmpeg demux/decode support.");
#endif
}

AcceleratorBackendStatus accelerator_backend_status() {
    AcceleratorBackendStatus status;
#if defined(_WIN32)
    status.d3d11_renderer_compiled = true;
#else
    status.blockers.push_back("D3D11 renderer requires a Windows build and is not compiled on this platform.");
#endif
    status.blockers.push_back("NVDEC GPU frame pool is not compiled yet; CUDA/NVDEC SDK integration is still required.");
    status.blockers.push_back("TensorRT inference backend is not compiled yet; TensorRT SDK integration and engine execution are still required.");
    return status;
}

std::string format_probe_result(const VideoProbeResult& result) {
    std::ostringstream output;
    output << "Video: " << result.path.string() << '\n';
    output << "Duration: " << std::fixed << std::setprecision(3) << result.duration_seconds << " seconds\n";
    output << "Resolution: " << result.video.width << 'x' << result.video.height << '\n';
    output << "FPS: " << std::fixed << std::setprecision(3) << result.video.fps << '\n';
    output << "Codec: " << result.video.codec_name << '\n';
    output << "Pixel format: " << result.video.pixel_format << '\n';
    output << "Audio: " << yes_no(result.has_audio) << '\n';
    output << "In-process FFmpeg: " << yes_no(ffmpeg_backend_compiled()) << '\n';
    return output.str();
}

std::string format_decode_smoke_result(const DecodeSmokeResult& result) {
    std::ostringstream output;
    output << format_probe_result(result.probe);
    output << "Decoded frames: " << result.decoded_frames << '\n';
    output << "First decoded frame format: " << result.first_frame_format << '\n';
    return output.str();
}

std::string format_accelerator_backend_status(const AcceleratorBackendStatus& status) {
    std::ostringstream output;
    output << "D3D11 renderer compiled: " << yes_no(status.d3d11_renderer_compiled) << '\n';
    output << "NVDEC frame pool compiled: " << yes_no(status.nvdec_compiled) << '\n';
    output << "TensorRT backend compiled: " << yes_no(status.tensor_rt_compiled) << '\n';
    if (!status.blockers.empty()) {
        output << "Blockers:\n";
        for (const auto& blocker : status.blockers) {
            output << "- " << blocker << '\n';
        }
    }
    return output.str();
}

}  // namespace focus_video
