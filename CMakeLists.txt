cmake_minimum_required(VERSION 3.15)
project(Axiom VERSION 0.4.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Find Dependencies ---
find_package(CURL REQUIRED)

# --- Compiler Flags ---
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    
    # Sanitizers for Debug builds
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
endif()

# --- Sources ---
file(GLOB_RECURSE SOURCES 
    "cmd/axiom/*.cpp"
    "internal/*.cpp"
    "internal/*.c"
    "internal/**/*.cpp"
    "internal/**/*.c"
)

# --- Include Directories ---
include_directories(internal pkg configs)

# --- Executable ---
add_executable(axiom ${SOURCES})

# --- Link Libraries ---
target_link_libraries(axiom PRIVATE CURL::libcurl)

# --- Installation ---
install(TARGETS axiom DESTINATION bin)
