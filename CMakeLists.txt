cmake_minimum_required(VERSION 3.28)
project("sfml-world" LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
  SFML
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG 2.6.x
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL SYSTEM
)
FetchContent_MakeAvailable(SFML)

set(TARGET_NAME "${PROJECT_NAME}-server")
add_executable("${TARGET_NAME}" src/server.cpp)
target_compile_features("${TARGET_NAME}" PRIVATE cxx_std_17)
target_link_libraries(
  "${TARGET_NAME}"
  PRIVATE sfml-graphics sfml-window sfml-system sfml-network
)

set(TARGET_NAME "${PROJECT_NAME}-client")
add_executable("${TARGET_NAME}" src/client.cpp)
target_compile_features("${TARGET_NAME}" PRIVATE cxx_std_17)
target_link_libraries(
  "${TARGET_NAME}"
  PRIVATE sfml-graphics sfml-window sfml-system sfml-network
)
