
cmake_minimum_required(VERSION 3.17)
project(texture_extractor)


# Download automatically, you can also just copy the conan.cmake file
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
  message(
    STATUS
      "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.15/conan.cmake"
       "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_run(REQUIRES stb/20190512@conan/stable BASIC_SETUP CMAKE_TARGETS
                BUILD missing)

add_executable(texture_extractor main.cpp)

target_link_libraries(texture_extractor CONAN_PKG::stb)
target_compile_features(texture_extractor PRIVATE cxx_std_17)
