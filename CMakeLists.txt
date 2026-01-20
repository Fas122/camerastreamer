cmake_minimum_required(VERSION 3.16)
project(media_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(PkgConfig REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavutil libswscale)

add_executable(media_server main.cpp frame_queue.h
  frame_queue.h)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    ${FFMPEG_LIBRARIES}
    Boost::filesystem
    ${OpenCV_LIBS}
)
