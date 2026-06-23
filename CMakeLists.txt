cmake_minimum_required(VERSION 3.16)
project(HelloContact VERSION 2.0 LANGUAGES CXX)

# ── C++ 标准 ──
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── 构建选项 ──
option(HELLO_CONTACT_BUILD_TERMINAL "Build terminal version" ON)
option(HELLO_CONTACT_BUILD_DESKTOP  "Build Qt desktop version" ON)
option(HELLO_CONTACT_BUILD_TESTS    "Build unit tests" ON)
option(HELLO_CONTACT_BUILD_LLM      "Build LLM module" OFF)

# ── 子目录 ──
add_subdirectory(core)

if(HELLO_CONTACT_BUILD_TERMINAL)
    add_subdirectory(apps/terminal)
endif()

if(HELLO_CONTACT_BUILD_DESKTOP)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
    add_subdirectory(apps/desktop)
endif()

if(HELLO_CONTACT_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(HELLO_CONTACT_BUILD_LLM)
    add_subdirectory(llm)
endif()
