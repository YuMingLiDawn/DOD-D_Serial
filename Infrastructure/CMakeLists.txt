# ##############################################################################
# Service子模块CMakeLists.txt 功能：Service服务层模块构建配置 架构：基础配置 -> 源文件定义 -> 目标创建 -> 包含目录
# -> 链接配置
# ##############################################################################

cmake_minimum_required(VERSION 3.10)

# ===============================================================================
# 第1层：模块基础配置
# ===============================================================================
project(Infrastructure)

message(STATUS "============================================================")
message(STATUS "  Configuring Infrastructure Module")
message(STATUS "============================================================")

# ===============================================================================
# 第2层：设置CMake模块路径（仅用于查找，不重新加载已在主CMakeLists中加载的模块）
# ===============================================================================
set(CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/../cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules ${CMAKE_MODULE_PATH})

# ===============================================================================
# 第3层：源文件配置
# ===============================================================================
include(
  ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules/InfrastructureSources.cmake)

# ===============================================================================
# 第4层：创建库目标
# ===============================================================================
add_library(Infrastructure ${INFRASTRUCTURE_SOURCES})

# ===============================================================================
# 第5层：包含目录配置
# ===============================================================================
target_include_directories(
  Infrastructure PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} # Config.h 和源代码
                        ${CMAKE_CURRENT_SOURCE_DIR}/.. # 上层目录（包含Config.h）
)

# Ensure Qt headers are available when compiling Infrastructure Qt imported
# targets provide include dirs and link properties; prefer linking to the
# imported target(s) directly so CMake handles include propagation.
# Infrastructure is a non-GUI module and currently only needs QtCore.

# ===============================================================================
# 第6层：链接库配置
# ===============================================================================
# GoogleLogger.h is part of Infrastructure public headers and includes
# <glog/logging.h>, so dod_glog must be PUBLIC to propagate include/link
# requirements to consumers.
if(UNIX AND NOT APPLE)
  target_link_libraries(Infrastructure PRIVATE Qt5::Core pthread dl)
else()
  target_link_libraries(Infrastructure PRIVATE Qt5::Core)
endif()
# QNetworkInterface is used in public headers (NicProvider.h) -> propagate Qt
# Network include/link to consumers
target_link_libraries(Infrastructure PUBLIC Qt5::Network)
target_link_libraries(Infrastructure PUBLIC dod_glog)

message(STATUS "Infrastructure module configured successfully")
message(STATUS "============================================================")
