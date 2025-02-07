cmake_minimum_required(VERSION 3.6)
set(CMAKE_CXX_STANDARD 17)
project(swaptube LANGUAGES CXX CUDA)

set(ENV{GLIBCXX_FORCE_NEW} 1)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(FFmpeg REQUIRED)
find_package(Cairo REQUIRED)
find_package(RSVG REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(PNG REQUIRED)
find_package(glm REQUIRED)
find_package(CUDA REQUIRED)

include_directories(${PROJECT_NAME} ${FFMPEG_INCLUDE_DIRS} ${RSVG_INCLUDE_DIRS})
include_directories("/usr/include/glib-2.0")
include_directories("/usr/lib/x86_64-linux-gnu/glib-2.0/include")
include_directories("/usr/include/gdk-pixbuf-2.0")
include_directories("/usr/include/cairo")
include_directories(${EIGEN3_INCLUDE_DIR})
include_directories(${PNG_INCLUDE_DIR})
include_directories(${GLM_INCLUDE_DIRS})
include_directories(${CUDA_INCLUDE_DIRS})

file(GLOB_RECURSE CUDA_SOURCES "src/CUDA/*.cu")

add_executable(${PROJECT_NAME} "src/projects/.active_project.cpp" ${CUDA_SOURCES})

target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES} ${SWSCALE_LIBRARIES} ${RSVG_LIBRARIES} cairo gobject-2.0 ${PNG_LIBRARIES} ${CUDA_LIBRARIES})

target_compile_options(${PROJECT_NAME} PRIVATE -Wreorder)

