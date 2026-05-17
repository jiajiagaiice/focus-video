cmake_minimum_required(VERSION 3.20)
project(focus_video VERSION 0.1.0 LANGUAGES CXX)

option(FOCUS_VIDEO_ENABLE_FFMPEG "Build in-process FFmpeg demux/decode support when development libraries are available" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FOCUS_VIDEO_HAS_FFMPEG OFF)
if(FOCUS_VIDEO_ENABLE_FFMPEG AND NOT CMAKE_CROSSCOMPILING)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(FFMPEG QUIET IMPORTED_TARGET libavformat libavcodec libavutil libswscale)
        if(FFMPEG_FOUND)
            set(FOCUS_VIDEO_HAS_FFMPEG ON)
        endif()
    endif()
endif()

add_library(focus_video_core
    src/focus_video.cpp
    src/image_super_resolution.cpp
    src/runtime_capabilities.cpp
    src/video_pipeline.cpp
)
target_include_directories(focus_video_core PUBLIC src)
target_compile_definitions(focus_video_core PUBLIC FOCUS_VIDEO_HAS_FFMPEG=$<BOOL:${FOCUS_VIDEO_HAS_FFMPEG}>)
if(FOCUS_VIDEO_HAS_FFMPEG)
    target_link_libraries(focus_video_core PUBLIC PkgConfig::FFMPEG)
endif()

add_executable(focus-video src/main.cpp)
target_link_libraries(focus-video PRIVATE focus_video_core)
if(MINGW)
    target_link_options(focus-video PRIVATE -static -static-libgcc -static-libstdc++)
endif()

install(TARGETS focus-video RUNTIME DESTINATION .)
install(FILES README.md LICENSE docs/windows-package.md DESTINATION .)

set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "FocusVideo")
set(CPACK_PACKAGE_VENDOR "Focus Video")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
if(WIN32)
    set(CPACK_PACKAGE_FILE_NAME "FocusVideo-${PROJECT_VERSION}-win64")
else()
    set(CPACK_PACKAGE_FILE_NAME "FocusVideo-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
endif()
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
include(CPack)

include(CTest)
if(BUILD_TESTING)
    add_executable(focus_video_tests tests/test_profiles.cpp)
    target_link_libraries(focus_video_tests PRIVATE focus_video_core)
    add_test(NAME focus_video_profiles COMMAND focus_video_tests)

    add_executable(focus_video_image_sr_tests tests/test_image_super_resolution.cpp)
    target_link_libraries(focus_video_image_sr_tests PRIVATE focus_video_core)
    add_test(NAME focus_video_image_super_resolution COMMAND focus_video_image_sr_tests)

    add_executable(focus_video_runtime_tests tests/test_runtime_capabilities.cpp)
    target_link_libraries(focus_video_runtime_tests PRIVATE focus_video_core)
    add_test(NAME focus_video_runtime_capabilities COMMAND focus_video_runtime_tests)

    add_executable(focus_video_pipeline_tests tests/test_video_pipeline.cpp)
    target_link_libraries(focus_video_pipeline_tests PRIVATE focus_video_core)
    add_test(NAME focus_video_pipeline COMMAND focus_video_pipeline_tests)
endif()
