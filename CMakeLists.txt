cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(SambarScamper VERSION 0.1)
set(CMAKE_CXX_STANDARD 20)

add_compile_options(-W)

find_package(SFML 2.5 COMPONENTS system window graphics audio REQUIRED)
find_package(box2d CONFIG REQUIRED)

file(GLOB_RECURSE SambarScamper_SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(sambar ${SambarScamper_SOURCE_FILES})
target_include_directories(sambar PRIVATE ${CMAKE_SOURCE_DIR}/include,
                                          ${CMAKE_SOURCE_DIR}/lib,
                                          ${CMAKE_SOURCE_DIR}/img)
target_link_libraries(sambar 
                      sfml-graphics
                      box2d)