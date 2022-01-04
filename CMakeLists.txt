cmake_minimum_required(VERSION 3.15)
project(mtap
  LANGUAGES CXX
)

add_library(mtap INTERFACE)
target_include_directories(mtap INTERFACE include)

install(DIRECTORY include/ TYPE INCLUDE)

if (MTAP_BUILD_TESTS)
  add_executable(example
    test/example.cpp
  )
  target_link_libraries(mtap)
endif()