cmake_minimum_required(VERSION 3.28)
project("sfml-world" LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
  SFML
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG 3.0.1
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL SYSTEM
)
FetchContent_MakeAvailable(SFML)

set(TARGET_NAME "${PROJECT_NAME}-server")
add_executable("${TARGET_NAME}" src/server.cpp)
target_compile_features("${TARGET_NAME}" PRIVATE cxx_std_23)
target_link_libraries(
  "${TARGET_NAME}"
  PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Network
)

set(TARGET_NAME "${PROJECT_NAME}-client")
add_executable("${TARGET_NAME}" src/client.cpp)
target_compile_features("${TARGET_NAME}" PRIVATE cxx_std_23)
target_link_libraries(
  "${TARGET_NAME}"
  PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Network
)
