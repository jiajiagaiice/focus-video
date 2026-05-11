#include "focus_video.hpp"

#include <algorithm>
#include <cctype>

namespace focus_video {
namespace {

std::string lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

}  // namespace

std::string to_string(UpscaleMode mode) {
    switch (mode) {
        case UpscaleMode::Off:
            return "Off";
        case UpscaleMode::Performance:
            return "Performance";
        case UpscaleMode::Balanced:
            return "Balanced";
        case UpscaleMode::Quality:
            return "Quality";
    }
    return "Unknown";
}

std::uint64_t pixels_per_second(const VideoInfo& video) {
    return static_cast<std::uint64_t>(video.width) * video.height *
           static_cast<std::uint64_t>(video.fps + 0.5);
}

std::uint64_t target_pixels(const VideoInfo& video) {
    return static_cast<std::uint64_t>(video.target_width) * video.target_height;
}

bool is_rtx_3060_ti_or_better_target(const GpuInfo& gpu) {
    const auto name = lower_copy(gpu.name);
    if (name.find("rtx 3060 ti") != std::string::npos) {
        return gpu.dedicated_memory_mb >= 8192 && gpu.supports_fp16;
    }

    const bool likely_rtx = name.find("rtx") != std::string::npos;
    const bool newer_generation = name.find("rtx 40") != std::string::npos ||
                                  name.find("rtx 50") != std::string::npos;
    const bool higher_30_series = name.find("rtx 3070") != std::string::npos ||
                                  name.find("rtx 3080") != std::string::npos ||
                                  name.find("rtx 3090") != std::string::npos;

    return likely_rtx && (newer_generation || higher_30_series) &&
           gpu.dedicated_memory_mb >= 8192 && gpu.supports_fp16;
}

PlaybackProfile choose_profile(const VideoInfo& video, const GpuInfo& gpu) {
    PlaybackProfile profile;
    profile.output_width = video.target_width;
    profile.output_height = video.target_height;

    if (!is_rtx_3060_ti_or_better_target(gpu) || !gpu.supports_tensor_rt) {
        profile.mode = UpscaleMode::Off;
        profile.model_scale = 1;
        profile.max_queue_depth = 1;
        profile.allow_frame_skip = false;
        profile.reason = "GPU below first-version target or TensorRT unavailable; keep playback stable.";
        return profile;
    }

    constexpr std::uint64_t p1440 = 2560ULL * 1440ULL;
    constexpr std::uint64_t p4k = 3840ULL * 2160ULL;
    if (target_pixels(video) > p4k) {
        profile.mode = UpscaleMode::Off;
        profile.model_scale = 1;
        profile.max_queue_depth = 1;
        profile.allow_frame_skip = true;
        profile.reason = "Target output exceeds first-version 4K budget; bypass SR to protect playback.";
        return profile;
    }

    const auto pps = pixels_per_second(video);
    constexpr std::uint64_t p720_60 = 1280ULL * 720ULL * 60ULL;
    constexpr std::uint64_t p1080_30 = 1920ULL * 1080ULL * 30ULL;
    constexpr std::uint64_t p1080_60 = 1920ULL * 1080ULL * 60ULL;

    if (pps <= p720_60 && target_pixels(video) <= p1440) {
        profile.mode = UpscaleMode::Quality;
        profile.max_queue_depth = 4;
        profile.reason = "Input load and 2K target fit a quality-oriented 2x model.";
    } else if (pps <= p1080_30) {
        profile.mode = UpscaleMode::Balanced;
        profile.max_queue_depth = 3;
        profile.reason = "3060 Ti target path for 1080p30 to 2K/4K real-time super resolution.";
    } else if (pps <= p1080_60) {
        profile.mode = UpscaleMode::Performance;
        profile.max_queue_depth = 2;
        profile.allow_frame_skip = true;
        profile.reason = "High frame-rate or 2K-class input; use a lighter model and allow SR frame skipping.";
    } else {
        profile.mode = UpscaleMode::Off;
        profile.model_scale = 1;
        profile.max_queue_depth = 1;
        profile.allow_frame_skip = true;
        profile.reason = "Input exceeds first-version 3060 Ti budget; bypass SR to protect playback.";
    }

    return profile;
}

}  // namespace focus_video
