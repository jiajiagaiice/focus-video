#include "runtime_capabilities.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

}  // namespace

int main() {
    const focus_video::RuntimeCapabilities none{};
    focus_video::PlaybackLaunchOptions playback;
    playback.video_path = std::filesystem::path("/definitely/missing/video.mp4");

    const auto blockers = focus_video::runtime_blockers(none, playback);
    assert(blockers.size() == 2);
    assert(contains(blockers[0], "Local video file does not exist"));
    assert(contains(blockers[1], "ffplay was not found"));

    playback.enable_tensor_rt_sr = true;
    playback.tensor_rt_engine_path = std::filesystem::path("/definitely/missing/model.engine");
    const auto sr_blockers = focus_video::runtime_blockers(none, playback);
    assert(sr_blockers.size() == 6);

    const auto report = focus_video::format_runtime_report(none, playback);
    assert(contains(report, "Focus Video runtime capability report"));
    assert(contains(report, "TensorRT SR requested: yes"));
    assert(contains(report, "frame bridge is not implemented yet"));

    std::cout << "runtime capability tests passed\n";
    return 0;
}
