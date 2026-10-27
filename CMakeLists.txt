cmake_minimum_required(VERSION 3.20)
project(Repo_Proyek LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Lokasi instalasi Qt 6.8.0 MinGW 64-bit
set(CMAKE_PREFIX_PATH "D:/Qt/6.8.0/mingw_64")

# Aktifkan Qt Meta-Object Compiler otomatis
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Microsoft ONNX Runtime C++ Integration
set(ONNXRUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/onnxruntime")
include_directories("${ONNXRUNTIME_DIR}/include")
link_directories("${ONNXRUNTIME_DIR}/lib")

set(SOURCES
    src/main.cpp
    src/models/AnnotationPoint.h
    src/core/VisionEngine.h
    src/core/VisionEngine.cpp
    src/core/OnnxInferenceEngine.h
    src/core/OnnxInferenceEngine.cpp
    src/ui/AnnotationCanvas.h
    src/ui/AnnotationCanvas.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

add_executable(Repo_Proyek ${SOURCES})

target_link_libraries(Repo_Proyek PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    "${ONNXRUNTIME_DIR}/lib/onnxruntime.lib"
)

# Salin onnxruntime.dll ke direktori output binary otomatis
add_custom_command(TARGET Repo_Proyek POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${ONNXRUNTIME_DIR}/lib/onnxruntime.dll"
    $<TARGET_FILE_DIR:Repo_Proyek>
)
