#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace focus_video {

struct RuntimeCapabilities {
    std::optional<std::filesystem::path> ffplay_path;
    std::optional<std::filesystem::path> ffmpeg_path;
    std::optional<std::filesystem::path> nvidia_smi_path;
    std::optional<std::filesystem::path> trtexec_path;
};

struct PlaybackLaunchOptions {
    std::filesystem::path video_path;
    std::uint32_t target_width = 3840;
    std::uint32_t target_height = 2160;
    bool enable_tensor_rt_sr = false;
    std::filesystem::path tensor_rt_engine_path;
};

RuntimeCapabilities detect_runtime_capabilities();
std::vector<std::string> runtime_blockers(const RuntimeCapabilities& capabilities,
                                           const PlaybackLaunchOptions& options);
std::string format_runtime_report(const RuntimeCapabilities& capabilities,
                                  const PlaybackLaunchOptions& options);
int play_local_video(const RuntimeCapabilities& capabilities, const PlaybackLaunchOptions& options);

}  // namespace focus_video
