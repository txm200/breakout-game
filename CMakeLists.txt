cmake_minimum_required(VERSION 3.10)
project(BreakoutGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 SDL2
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

add_executable(breakout main.cpp)

target_include_directories(breakout PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})
target_link_libraries(breakout ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})

# Windows 特殊配置
if(WIN32)
    set_target_properties(breakout PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# 安装规则
install(TARGETS breakout DESTINATION bin)