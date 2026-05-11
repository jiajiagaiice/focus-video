#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace focus_video {

struct VideoStreamInfo {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    double fps = 0.0;
    std::string codec_name;
    std::string pixel_format;
};

struct VideoProbeResult {
    std::filesystem::path path;
    double duration_seconds = 0.0;
    VideoStreamInfo video;
    bool has_audio = false;
};

struct DecodeSmokeResult {
    VideoProbeResult probe;
    std::uint32_t decoded_frames = 0;
    std::string first_frame_format;
};

struct AcceleratorBackendStatus {
    bool d3d11_renderer_compiled = false;
    bool nvdec_compiled = false;
    bool tensor_rt_compiled = false;
    std::vector<std::string> blockers;
};

bool ffmpeg_backend_compiled();
VideoProbeResult probe_video(const std::filesystem::path& path);
DecodeSmokeResult decode_video_smoke(const std::filesystem::path& path, std::uint32_t max_frames);
AcceleratorBackendStatus accelerator_backend_status();
std::string format_probe_result(const VideoProbeResult& result);
std::string format_decode_smoke_result(const DecodeSmokeResult& result);
std::string format_accelerator_backend_status(const AcceleratorBackendStatus& status);

}  // namespace focus_video
