#pragma once

#include <cstdint>
#include <string>

namespace focus_video {

enum class UpscaleMode {
    Off,
    Performance,
    Balanced,
    Quality,
};

struct VideoInfo {
    std::uint32_t width = 1920;
    std::uint32_t height = 1080;
    double fps = 30.0;
    std::uint32_t target_width = 3840;
    std::uint32_t target_height = 2160;
};

struct GpuInfo {
    std::string name = "Unknown GPU";
    std::uint32_t dedicated_memory_mb = 8192;
    bool supports_fp16 = true;
    bool supports_tensor_rt = true;
};

struct PlaybackProfile {
    UpscaleMode mode = UpscaleMode::Balanced;
    std::uint32_t model_scale = 2;
    std::uint32_t output_width = 3840;
    std::uint32_t output_height = 2160;
    std::uint32_t max_queue_depth = 3;
    bool allow_frame_skip = false;
    std::string reason;
};

std::string to_string(UpscaleMode mode);
std::uint64_t pixels_per_second(const VideoInfo& video);
std::uint64_t target_pixels(const VideoInfo& video);
bool is_rtx_3060_ti_or_better_target(const GpuInfo& gpu);
PlaybackProfile choose_profile(const VideoInfo& video, const GpuInfo& gpu);

}  // namespace focus_video
