# Minimum CMake version required
cmake_minimum_required(VERSION 3.5)

# Enable export commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Project name and version
project(stdui VERSION 0.1)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable error if a C++ initializer struct list is in wrong order and link statically to libgcc and libstdc++
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=reorder -static-libgcc -static-libstdc++")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=reorder")
endif()

if (EMSCRIPTEN)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O3 -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1")
    set(CMAKE_EXECUTABLE_SUFFIX ".html") # This line is used to set your executable to build with the emscripten html template so that you can directly open it.
endif ()

# # # # # # # # # # #
#
#  | |  / /__  ____  ____/ /___  _____
#  | | / / _ \/ __ \/ __  / __ \/ ___/
#  | |/ /  __/ / / / /_/ / /_/ / /
#  |___/\___/_/ /_/\__,_/\____/_/
#
#  > All Header & Source Files that come from
#  > external sources or are mostly copy.
#

include_directories(./vendor/GLES2)
include_directories(./vendor/KHR)
include_directories(./vendor)

include(FetchContent)

# Download and setup raylib
add_definitions(-Draylib_USE_STATIC_LIBS)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

# Download and setup hjson
FetchContent_Declare(
  hjson
  GIT_REPOSITORY https://github.com/hjson/hjson-cpp.git
  GIT_TAG 2.5
)
FetchContent_MakeAvailable(hjson)

# Download and setup litehtml
FetchContent_Declare(
  litehtml
  GIT_REPOSITORY https://github.com/litehtml/litehtml.git
  GIT_TAG d4453f5d4e03cd4d902b867ca553d0ad81b09939 # I had some problems with the latest version, so I used this one
)
set(LITEHTML_BUILD_TESTING OFF CACHE BOOL "Skip building tests" FORCE)
FetchContent_MakeAvailable(litehtml)

# I had some issues with the compiler warnings, so I disabled them
if(TARGET litehtml)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
        target_compile_options(litehtml PRIVATE
      -Wno-error
      -Wno-reorder-ctor
      -Wno-switch
    )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(litehtml PRIVATE -Wno-error -Wno-reorder -Wno-switch)
    endif()
endif()

# Download and setup nativefiledialog
FetchContent_Declare(
  nativefiledialog
  GIT_REPOSITORY https://github.com/mlabbe/nativefiledialog.git
  GIT_TAG master
)
FetchContent_GetProperties(nativefiledialog)
if(NOT nativefiledialog_POPULATED)
  FetchContent_Populate(nativefiledialog)
endif()

if (APPLE)
  add_library(nativefiledialog STATIC
    ${nativefiledialog_SOURCE_DIR}/src/nfd_common.c
    ${nativefiledialog_SOURCE_DIR}/src/nfd_cocoa.m
  )
  target_link_libraries(nativefiledialog "-framework AppKit")
elseif (WIN32)
  add_library(nativefiledialog STATIC
    ${nativefiledialog_SOURCE_DIR}/src/nfd_common.c
    ${nativefiledialog_SOURCE_DIR}/src/nfd_win.cpp
  )
elseif (UNIX)
  add_library(nativefiledialog STATIC
    ${nativefiledialog_SOURCE_DIR}/src/nfd_common.c
    ${nativefiledialog_SOURCE_DIR}/src/nfd_gtk.c
  )
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
  target_include_directories(nativefiledialog PRIVATE ${GTK3_INCLUDE_DIRS})
  target_link_libraries(nativefiledialog ${GTK3_LIBRARIES})
endif()
target_include_directories(nativefiledialog PUBLIC ${nativefiledialog_SOURCE_DIR}/src/include)

# Use vendored ImHTML (patched: CustomElement calls set_tagName so CSS selectors match)
# Add include directories
include_directories(./vendor/imgui)
include_directories(./vendor/imhtml)
set(code_vendor_imgui
        vendor/imgui/imgui.cpp
        vendor/imgui/imgui_draw.cpp
        vendor/imgui/imgui_demo.cpp
        vendor/imgui/imgui_widgets.cpp
        vendor/imgui/imgui_tables.cpp
        vendor/imgui/rlImGui.cpp
        vendor/imhtml/imhtml.cpp)

# Defines
add_definitions(-DNO_FONT_AWESOME)
add_definitions(-DIMGUI_DEFINE_MATH_OPERATORS)

# # # # # # # # # # #
#
#      ____        _ __    __
#     / __ )__  __(_) /___/ /
#    / __  / / / / / / __  /
#   / /_/ / /_/ / / / /_/ /
#  /_____/\__,_/_/_/\__,_/
#
#  > Build all the things!
#

# Find all .cpp files in the src directory
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Includes
include_directories(./src/)

# Add the executable
add_executable(stdui main.cpp ${SOURCES} ${code_vendor_imgui})

# Link dependencies
target_link_libraries(stdui raylib litehtml hjson nativefiledialog)

if (APPLE)
    # Link ./libEGL.dylib and ./libGLESv2.dylib
    target_link_libraries(stdui ${CMAKE_CURRENT_SOURCE_DIR}/libEGL.dylib)
    target_link_libraries(stdui ${CMAKE_CURRENT_SOURCE_DIR}/libGLESv2.dylib)
endif()
