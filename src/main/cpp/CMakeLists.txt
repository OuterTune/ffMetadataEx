# CMakeLists.txt
cmake_minimum_required(VERSION 3.31.6)
project(ffMetadataEx)

# flags based on https://github.com/Kyant0/taglib
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -fvisibility=hidden -flto -fdata-sections -ffunction-sections -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -fvisibility=hidden -flto -fdata-sections -ffunction-sections -fomit-frame-pointer")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flto -Wl,--exclude-libs,ALL -Wl,--gc-sections -s -Wl,--pack-dyn-relocs=android -Wl,--build-id=none -Wl,-z,max-page-size=16384")

set(VISIBILITY_HIDDEN ON)
set(BUILD_BINDINGS OFF)
set(BUILD_TESTING OFF)

set(ffmpeg_libs_names
        avformat avutil avcodec swresample swscale)

set(ffmpeg_dir ${CMAKE_SOURCE_DIR}/../../../ffmpeg-android-maker/output)
set(ffmpeg_libs ${ffmpeg_dir}/lib/${ANDROID_ABI})

foreach (ffmpeg_lib_name ${ffmpeg_libs_names})
    add_library(${ffmpeg_lib_name} SHARED IMPORTED)
    set_target_properties(${ffmpeg_lib_name}
            PROPERTIES IMPORTED_LOCATION
            ${ffmpeg_libs}/lib${ffmpeg_lib_name}.so)
endforeach ()

# Include FFmpeg headers
include_directories(${ffmpeg_dir}/include/${ANDROID_ABI})

add_library(${CMAKE_PROJECT_NAME} SHARED
        ffMetaExJni.cpp
        ffmain.cpp
        ffcommon.cpp
        ffaudio.cpp)

# Link FFmpeg libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
        log
        android
        avformat
        avutil
        avcodec
        swresample
        swscale
)

# Set the output directory for the .so file
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI})