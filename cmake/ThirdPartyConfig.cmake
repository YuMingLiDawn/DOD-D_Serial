# ==============================================================================
# 第三方依赖 spdlog（日志）：仓库内置 > 系统安装包 > 在线拉取 - 内置版：别人 clone 下来无需联网、无需额外安装，直接能编译 -
# 换日志库时：删掉这段 + 重写 Utility/Log.cpp（业务代码零改动） Threads：spdlog 用到 std::thread，Linux
# 需显式链接 pthread
# ==============================================================================

find_package(Threads REQUIRED)

set(_spdlog_vendor "${PROJECT_SOURCE_DIR}/third_party/spdlog")
set(DOD_SPDLOG_TARGET "")

if(EXISTS "${_spdlog_vendor}/include/spdlog/spdlog.h")
  # 内置版走 header-only 模式，不需要单独编译库
  add_library(dod_spdlog INTERFACE)
  target_include_directories(dod_spdlog SYSTEM
                             INTERFACE "${_spdlog_vendor}/include")
  target_compile_definitions(dod_spdlog INTERFACE SPDLOG_HEADER_ONLY)
  set(DOD_SPDLOG_TARGET dod_spdlog)
  message(STATUS "spdlog: 使用仓库内置版本 (third_party/spdlog)")
else()
  find_package(spdlog CONFIG QUIET)
  if(spdlog_FOUND)
    set(DOD_SPDLOG_TARGET spdlog::spdlog)
    message(STATUS "spdlog: 使用系统安装版本 ${spdlog_VERSION}")
  else()
    include(FetchContent)
    message(STATUS "spdlog: 本地未找到，尝试在线拉取 v1.14.1")
    set(SPDLOG_BUILD_EXAMPLE
        OFF
        CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_TESTS
        OFF
        CACHE BOOL "" FORCE)
    FetchContent_Declare(
      spdlog
      URL https://github.com/gabime/spdlog/archive/refs/tags/v1.14.1.tar.gz
          DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(spdlog)
    set(DOD_SPDLOG_TARGET spdlog::spdlog)
  endif()
endif()
