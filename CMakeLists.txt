cmake_minimum_required(VERSION 3.10)
project(BreakoutGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===== SDL2 查找 =====
# 策略1: 使用 pkg-config (Linux/macOS)
# 策略2: 使用 vcpkg 的 cmake config (Windows vcpkg)
# 策略3: 使用 FindSDL2 模块 (fallback)

find_package(PkgConfig QUIET)

if(PkgConfig_FOUND)
    pkg_check_modules(SDL2 QUIET sdl2)
    pkg_check_modules(SDL2_TTF QUIET SDL2_ttf)
endif()

if(NOT SDL2_FOUND)
    find_package(SDL2 QUIET CONFIG)
    if(SDL2_FOUND)
        # vcpkg 安装的 SDL2 提供 SDL2::SDL2 目标
        set(SDL2_TARGET SDL2::SDL2)
    endif()
endif()

if(NOT SDL2_FOUND)
    # 最后的 fallback: 手动查找
    find_path(SDL2_INCLUDE_DIRS SDL.h
        PATH_SUFFIXES SDL2
    )
    find_library(SDL2_LIBRARIES SDL2)
    if(NOT SDL2_INCLUDE_DIRS OR NOT SDL2_LIBRARIES)
        message(FATAL_ERROR "找不到 SDL2！请安装 SDL2 开发库")
    endif()
    set(SDL2_FOUND TRUE)
endif()

# SDL2_ttf
if(NOT SDL2_TTF_FOUND)
    find_package(SDL2_ttf QUIET CONFIG)
    if(SDL2_ttf_FOUND)
        set(SDL2_TTF_TARGET SDL2_ttf::SDL2_ttf)
    endif()
endif()

if(NOT SDL2_TTF_FOUND)
    find_path(SDL2_TTF_INCLUDE_DIRS SDL_ttf.h
        PATH_SUFFIXES SDL2
    )
    find_library(SDL2_TTF_LIBRARIES SDL2_ttf)
    if(NOT SDL2_TTF_INCLUDE_DIRS OR NOT SDL2_TTF_LIBRARIES)
        message(FATAL_ERROR "找不到 SDL2_ttf！请安装 SDL2_ttf 开发库")
    endif()
    set(SDL2_TTF_FOUND TRUE)
endif()

message(STATUS "SDL2: ${SDL2_INCLUDE_DIRS} ${SDL2_LIBRARIES}")
message(STATUS "SDL2_ttf: ${SDL2_TTF_INCLUDE_DIRS} ${SDL2_TTF_LIBRARIES}")

# ===== 可执行文件 =====
add_executable(breakout main.cpp)

target_include_directories(breakout PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
)

if(SDL2_TARGET)
    target_link_libraries(breakout PRIVATE ${SDL2_TARGET})
else()
    target_link_libraries(breakout PRIVATE ${SDL2_LIBRARIES})
endif()

if(SDL2_TTF_TARGET)
    target_link_libraries(breakout PRIVATE ${SDL2_TTF_TARGET})
else()
    target_link_libraries(breakout PRIVATE ${SDL2_TTF_LIBRARIES})
endif()

# Windows 特殊配置
if(WIN32)
    set_target_properties(breakout PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# 安装规则
install(TARGETS breakout DESTINATION bin)