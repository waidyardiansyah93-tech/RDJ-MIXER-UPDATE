cmake_minimum_required(VERSION 3.22)
project(DM16 VERSION 0.4.1)

include(FetchContent)

# JUCE 8.0.11 includes the ASIO SDK headers needed by JUCE's ASIO backend.
# This avoids requiring users to download/configure a separate Steinberg SDK.
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.11
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# Main digital mixer
juce_add_gui_app(DM16 PRODUCT_NAME "DM-16 Digital Mixer" COMPANY_NAME "DM16")
juce_generate_juce_header(DM16)
target_sources(DM16 PRIVATE Source/Main.cpp)

# RDJ MIXER application icon for Windows (EXE, Explorer and taskbar).
if(WIN32)
  target_sources(DM16 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/RDJ_Mixer.rc"
  )
endif()
target_compile_features(DM16 PRIVATE cxx_std_17)
target_compile_definitions(DM16 PRIVATE
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_PLUGINHOST_VST3=1
  JUCE_PLUGINHOST_VST=0
  JUCE_PLUGINHOST_AU=0
  JUCE_PLUGINHOST_LV2=0)

# Enable JUCE's native Windows ASIO device backend. JUCE enumerates the ASIO
# drivers registered in Windows; each driver is shown as a selectable device.
if(WIN32)
  target_compile_definitions(DM16 PRIVATE
    JUCE_ASIO=1
    JUCE_ASIO_USE_EXTERNAL_SDK=0)
  message(STATUS "RDJ MIXER: JUCE ASIO backend ENABLED")
else()
  message(STATUS "RDJ MIXER: ASIO backend is Windows-only")
endif()

target_link_libraries(DM16 PRIVATE
  juce::juce_audio_utils
  juce::juce_audio_processors
  juce::juce_audio_devices
  juce::juce_gui_extra
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags)

# Separate VST3 scanner. A faulty third-party plugin can crash this process
# without taking down the mixer process.
juce_add_console_app(RDJ_VST3_Scanner PRODUCT_NAME "RDJ VST3 Scanner")
juce_generate_juce_header(RDJ_VST3_Scanner)
target_sources(RDJ_VST3_Scanner PRIVATE Source/VST3Scanner.cpp)
target_compile_features(RDJ_VST3_Scanner PRIVATE cxx_std_17)
target_compile_definitions(RDJ_VST3_Scanner PRIVATE
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_PLUGINHOST_VST3=1
  JUCE_PLUGINHOST_VST=0
  JUCE_PLUGINHOST_AU=0
  JUCE_PLUGINHOST_LV2=0)
target_link_libraries(RDJ_VST3_Scanner PRIVATE
  juce::juce_audio_utils
  juce::juce_audio_processors
  juce::juce_audio_devices
  juce::juce_gui_extra
  juce::juce_recommended_config_flags
  juce::juce_recommended_warning_flags)

# Place the scanner beside the mixer executable so Main.cpp can launch it.
# $<CONFIG> keeps Visual Studio Debug/Release outputs in matching folders.
set_target_properties(RDJ_VST3_Scanner PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/DM16_artefacts/$<CONFIG>"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/DM16_artefacts/$<CONFIG>"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/DM16_artefacts/$<CONFIG>")
