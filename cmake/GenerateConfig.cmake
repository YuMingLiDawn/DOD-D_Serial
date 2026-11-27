# ==============================================================================
# Config.h 生成脚本（由 cmake -P 执行，每次编译跑一次）
#
# 入参（由 cmake/VersionConfig.cmake 传入）： SRC_DIR      源码根目录（探测 git 用） CFG_IN
# Config.h.in 路径 CFG_OUT      生成的 Config.h 路径 PRJ_NAME     工程名 VER_MAJOR /
# VER_MINOR / VER_MICRO / VER_BUILD
#
# 关键点：configure_file 在“生成内容与现有文件完全相同”时不会改动目标文件， 所以内容没变时不会触发下游重新编译，只有 BUILD_TIME
# / git 变了才更新。
# ==============================================================================
if(NOT DEFINED CFG_IN OR NOT DEFINED CFG_OUT)
  message(FATAL_ERROR "GenerateConfig: 必须传入 CFG_IN 和 CFG_OUT")
endif()

# --- git 信息：不是仓库 / 没装 git 时降级为 unknown，不阻断编译 ---
set(GIT_COMMIT_HASH "unknown")
set(GIT_BRANCH "unknown")
if(DEFINED SRC_DIR AND EXISTS "${SRC_DIR}/.git")
  execute_process(
    COMMAND git -C "${SRC_DIR}" rev-parse --short=7 HEAD
    OUTPUT_VARIABLE _git_hash
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
    RESULT_VARIABLE _rc_hash)
  execute_process(
    COMMAND git -C "${SRC_DIR}" rev-parse --abbrev-ref HEAD
    OUTPUT_VARIABLE _git_branch
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
    RESULT_VARIABLE _rc_branch)
  if(_rc_hash EQUAL 0 AND _git_hash)
    set(GIT_COMMIT_HASH "${_git_hash}")
  endif()
  if(_rc_branch EQUAL 0 AND _git_branch)
    set(GIT_BRANCH "${_git_branch}")
  endif()
endif()

# --- 构建时间：这一项每次编译都会变，是“自动更新”的直接体现 ---
string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S")

set(TITLE "AMS")
set(DESCRIPTION "AMS Wireless Control")
set(BIN_NAME "${PRJ_NAME}")
set(PROJECT_NAME "${PRJ_NAME}")
set(VERSION_MAJOR "${VER_MAJOR}")
set(VERSION_MINOR "${VER_MINOR}")
set(VERSION_MICRO "${VER_MICRO}")
set(VERSION_BUILD "${VER_BUILD}")
set(VERSION_STRING "${VER_MAJOR}.${VER_MINOR}.${VER_MICRO}.${VER_BUILD}")

configure_file("${CFG_IN}" "${CFG_OUT}" @ONLY)
