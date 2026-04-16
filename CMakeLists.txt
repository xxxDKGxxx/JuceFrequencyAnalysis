cmake_minimum_required(VERSION 3.22)

project(JUCEAIPDProj2 VERSION 0.0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0
)
FetchContent_MakeAvailable(juce)

FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_PROGRESS   TRUE
  GIT_SHALLOW    TRUE
  GIT_TAG        v1.91.9b-docking)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(imgui_juce
  GIT_REPOSITORY https://github.com/Krasjet/imgui_juce
  GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(imgui_juce)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)

FetchContent_Declare(implot
  GIT_REPOSITORY https://github.com/epezent/implot.git
  GIT_TAG        v0.16)
FetchContent_MakeAvailable(implot)

add_library(implot STATIC
  ${implot_SOURCE_DIR}/implot.cpp
  ${implot_SOURCE_DIR}/implot_items.cpp
  ${implot_SOURCE_DIR}/implot_demo.cpp)

target_include_directories(implot PUBLIC ${implot_SOURCE_DIR})

target_link_libraries(implot PUBLIC imgui)

juce_add_gui_app(App
    PRODUCT_NAME "Frequency Analyzer")

file(GLOB_RECURSE ALL_SOURCE_FILES CONFIGURE_DEPENDS
    "Source/*.cpp"
    "Source/*.h"
)

target_sources(App PRIVATE ${ALL_SOURCE_FILES})

target_compile_definitions(App PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_include_directories(imgui
  PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends)

target_link_libraries(App PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_formats
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_events
    juce::juce_core
    
    juce::juce_opengl
    juce::juce_dsp
    imgui
    imgui_impl_juce
    implot)

juce_generate_juce_header(App)
