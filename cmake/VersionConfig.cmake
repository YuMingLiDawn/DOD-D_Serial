# ==============================================================================
# 版本与构建信息（Config.h）
#
# 目的：每次编译都重新生成 Config.h，让 BUILD_TIME / GIT_COMMIT_HASH 始终是最新的。
# 做法：add_custom_target(... ALL) 挂到 ALL 上，make 每次都会执行它（custom target 永远被视为 out of
# date），脚本内部再用 configure_file 判断内容是否变化。
# 产物：<build>/generated/Config.h（不污染源码树，也不需要 git 忽略）
# ==============================================================================

set(DOD_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${DOD_GENERATED_DIR}")

# 版本第 4 位用 git 提交次数（没有 git 时退化为 0）
execute_process(
  COMMAND git -C "${PROJECT_SOURCE_DIR}" rev-list --count HEAD
  OUTPUT_VARIABLE _dod_commit_count
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
  RESULT_VARIABLE _dod_git_rc)
if(NOT _dod_git_rc EQUAL 0 OR NOT _dod_commit_count MATCHES "^[0-9]+$")
  set(_dod_commit_count 0)
endif()

add_custom_target(
  DodGenerateConfig ALL
  COMMAND
    ${CMAKE_COMMAND} "-DSRC_DIR=${PROJECT_SOURCE_DIR}"
    "-DCFG_IN=${PROJECT_SOURCE_DIR}/Config.h.in"
    "-DCFG_OUT=${DOD_GENERATED_DIR}/Config.h" "-DPRJ_NAME=${PROJECT_NAME}"
    "-DVER_MAJOR=${PROJECT_VERSION_MAJOR}"
    "-DVER_MINOR=${PROJECT_VERSION_MINOR}"
    "-DVER_MICRO=${PROJECT_VERSION_PATCH}" "-DVER_BUILD=${_dod_commit_count}" -P
    "${CMAKE_CURRENT_LIST_DIR}/GenerateConfig.cmake"
  BYPRODUCTS "${DOD_GENERATED_DIR}/Config.h"
  COMMENT "生成 Config.h（v${PROJECT_VERSION}.${_dod_commit_count} / git / 构建时间）"
  VERBATIM)

# 编译前先生成，并让源码能 #include "Config.h"
add_dependencies(${PROJECT_NAME} DodGenerateConfig)
target_include_directories(${PROJECT_NAME} PRIVATE "${DOD_GENERATED_DIR}")

# 模板本身被改动时重新 configure
set_property(
  DIRECTORY
  APPEND
  PROPERTY CMAKE_CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/Config.h.in")
