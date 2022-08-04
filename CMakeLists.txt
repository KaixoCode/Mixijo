cmake_minimum_required (VERSION 3.0)
project (Mixijo)

set (CMAKE_CXX_STANDARD 23)

add_subdirectory(libs)

set(SRC "${Mixijo_SOURCE_DIR}/")

file(GLOB_RECURSE SOURCE
  "${SRC}source/*.cpp"
  "${SRC}include/*.hpp"
)

add_executable(Mixijo
  ${SOURCE}
)

target_include_directories(Mixijo PUBLIC
  libs/Guijo/include
  libs/Guijo/libs
  libs/Midijo/include
  libs/Audijo/include
  ${SRC}include
)

source_group(TREE ${SRC} FILES ${SOURCE})

target_precompile_headers(Mixijo PUBLIC
  "${SRC}include/pch.hpp"
)

target_link_libraries(Mixijo
  Guijo
  Midijo
  Audijo
)