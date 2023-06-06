cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(SambarScamper VERSION 0.1)
set(CMAKE_CXX_STANDARD 20)

add_compile_options(-W)

set(SFML_COMMIT 2f11710abc5aa478503a7ff3f9e654bd2078ebab)
set(BOX2D_CMAKE_COMMIT master)


if(WIN32)
    # static linking of important libraries on windows
    set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -lwsock32 -lws2_32 ${CMAKE_CXX_STANDARD_LIBRARIES}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -lpthread -Wl,--no-whole-archive")
endif()

include(FetchContent)

# Fetches SFML dependency and loads its CMakeLists.txt
FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG ${SFML_COMMIT})
FetchContent_MakeAvailable(SFML)

# this creates the variables SFML_SOURCE_DIR, SFML_BINARY_DIR and SFML_POPULATED
# then SFML_SOURCE_DIR is used below to include the header files
FetchContent_GetProperties(SFML)

# Fetches Box2D dependency and loads its CMakeLists.txt
FetchContent_Declare(Box2D
        GIT_REPOSITORY https://github.com/WheezyWiseWizards/Box2D-cmake.git
        GIT_TAG ${BOX2D_CMAKE_COMMIT})
FetchContent_MakeAvailable(Box2D)
FetchContent_GetProperties(Box2D SOURCE_DIR Box2D_SOURCE_DIR)


file(GLOB_RECURSE SambarScamper_SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(sambar ${SambarScamper_SOURCE_FILES})
target_include_directories(sambar PRIVATE ${CMAKE_SOURCE_DIR}/include,
                                          ${CMAKE_SOURCE_DIR}/lib,
                                          ${CMAKE_SOURCE_DIR}/img)
target_link_libraries(sambar 
                      sfml-graphics
                      WIZ::Box2D)
