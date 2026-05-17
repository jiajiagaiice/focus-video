#include "runtime_capabilities.hpp"

#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace focus_video {
namespace {

#ifdef _WIN32
constexpr char path_separator = ';';
constexpr char executable_suffix[] = ".exe";
#else
constexpr char path_separator = ':';
constexpr char executable_suffix[] = "";
#endif

std::optional<std::filesystem::path> find_on_path(const std::string& executable) {
    const char* raw_path = std::getenv("PATH");
    if (raw_path == nullptr) {
        return std::nullopt;
    }

    std::stringstream stream(raw_path);
    std::string directory;
    while (std::getline(stream, directory, path_separator)) {
        if (directory.empty()) {
            continue;
        }
        auto candidate = std::filesystem::path(directory) / (executable + executable_suffix);
        std::error_code error;
        if (std::filesystem::exists(candidate, error) && !std::filesystem::is_directory(candidate, error)) {
            return candidate;
        }
#ifndef _WIN32
        candidate = std::filesystem::path(directory) / executable;
        if (std::filesystem::exists(candidate, error) && !std::filesystem::is_directory(candidate, error)) {
            return candidate;
        }
#endif
    }
    return std::nullopt;
}

std::string present_or_missing(const std::optional<std::filesystem::path>& path) {
    return path ? path->string() : "missing";
}

std::string shell_quote(const std::filesystem::path& path) {
    const auto value = path.string();
#ifdef _WIN32
    std::string quoted = "\"";
    for (const char ch : value) {
        if (ch == '"') {
            quoted += "\\\"";
        } else {
            quoted += ch;
        }
    }
    quoted += "\"";
    return quoted;
#else
    std::string quoted = "'";
    for (const char ch : value) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted += ch;
        }
    }
    quoted += "'";
    return quoted;
#endif
}

}  // namespace

RuntimeCapabilities detect_runtime_capabilities() {
    return RuntimeCapabilities{
        find_on_path("ffplay"),
        find_on_path("ffmpeg"),
        find_on_path("nvidia-smi"),
        find_on_path("trtexec"),
    };
}

std::vector<std::string> runtime_blockers(const RuntimeCapabilities& capabilities,
                                           const PlaybackLaunchOptions& options) {
    std::vector<std::string> blockers;
    std::error_code error;
    if (!options.video_path.empty() &&
        (!std::filesystem::exists(options.video_path, error) || std::filesystem::is_directory(options.video_path, error))) {
        blockers.push_back("Local video file does not exist: " + options.video_path.string());
    }

    if (!capabilities.ffplay_path) {
        blockers.push_back("ffplay was not found on PATH; install FFmpeg with ffplay to run the local playback backend.");
    }

    if (options.enable_tensor_rt_sr) {
        if (!capabilities.nvidia_smi_path) {
            blockers.push_back("nvidia-smi was not found on PATH; an NVIDIA runtime/driver check is required before TensorRT SR.");
        }
        if (!capabilities.trtexec_path) {
            blockers.push_back("trtexec was not found on PATH; install TensorRT and expose trtexec before enabling real TensorRT SR.");
        }
        if (options.tensor_rt_engine_path.empty()) {
            blockers.push_back("No TensorRT engine path was provided; pass --trt-engine MODEL.engine for real SR playback.");
        } else if (!std::filesystem::exists(options.tensor_rt_engine_path, error) ||
                   std::filesystem::is_directory(options.tensor_rt_engine_path, error)) {
            blockers.push_back("TensorRT engine file does not exist: " + options.tensor_rt_engine_path.string());
        }
        blockers.push_back("The FFmpeg/NVDEC -> TensorRT -> renderer frame bridge is not implemented yet, so real-time TensorRT SR playback is still blocked even when the tools are installed.");
    }

    return blockers;
}

std::string format_runtime_report(const RuntimeCapabilities& capabilities,
                                  const PlaybackLaunchOptions& options) {
    std::ostringstream output;
    output << "Focus Video runtime capability report\n";
    output << "ffplay: " << present_or_missing(capabilities.ffplay_path) << '\n';
    output << "ffmpeg: " << present_or_missing(capabilities.ffmpeg_path) << '\n';
    output << "nvidia-smi: " << present_or_missing(capabilities.nvidia_smi_path) << '\n';
    output << "trtexec: " << present_or_missing(capabilities.trtexec_path) << '\n';
    output << "video: " << (options.video_path.empty() ? std::string("not provided") : options.video_path.string()) << '\n';
    output << "target: " << options.target_width << 'x' << options.target_height << '\n';
    output << "TensorRT SR requested: " << (options.enable_tensor_rt_sr ? "yes" : "no") << '\n';
    if (!options.tensor_rt_engine_path.empty()) {
        output << "TensorRT engine: " << options.tensor_rt_engine_path.string() << '\n';
    }

    const auto blockers = runtime_blockers(capabilities, options);
    if (blockers.empty()) {
        output << "status: ready for local video playback without TensorRT SR\n";
    } else {
        output << "status: blocked\n";
        for (const auto& blocker : blockers) {
            output << "- " << blocker << '\n';
        }
    }
    return output.str();
}

int play_local_video(const RuntimeCapabilities& capabilities, const PlaybackLaunchOptions& options) {
    if (options.video_path.empty()) {
        throw std::runtime_error("Playback is not ready:\n- No local video path was provided.\n");
    }

    const auto blockers = runtime_blockers(capabilities, options);
    if (!blockers.empty()) {
        std::ostringstream message;
        message << "Playback is not ready:\n";
        for (const auto& blocker : blockers) {
            message << "- " << blocker << '\n';
        }
        throw std::runtime_error(message.str());
    }

    const std::string command = shell_quote(*capabilities.ffplay_path) +
                                " -autoexit -hide_banner " + shell_quote(options.video_path);
    return std::system(command.c_str());
}

}  // namespace focus_video
