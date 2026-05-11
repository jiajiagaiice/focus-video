cmake_minimum_required(VERSION 3.20)
project(focus_video VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(focus_video_core
    src/focus_video.cpp
    src/image_super_resolution.cpp
)
target_include_directories(focus_video_core PUBLIC src)

add_executable(focus-video src/main.cpp)
target_link_libraries(focus-video PRIVATE focus_video_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(focus_video_tests tests/test_profiles.cpp)
    target_link_libraries(focus_video_tests PRIVATE focus_video_core)
    add_test(NAME focus_video_profiles COMMAND focus_video_tests)

    add_executable(focus_video_image_sr_tests tests/test_image_super_resolution.cpp)
    target_link_libraries(focus_video_image_sr_tests PRIVATE focus_video_core)
    add_test(NAME focus_video_image_super_resolution COMMAND focus_video_image_sr_tests)
endif()
