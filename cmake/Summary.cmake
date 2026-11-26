# ==============================================================================
# 配置结论汇总（出错时先看这一段）
# ==============================================================================

message(STATUS "============================================================")
message(STATUS "  项目        : ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  Qt 前缀     : ${CMAKE_PREFIX_PATH}")
message(STATUS "  构建类型    : ${CMAKE_BUILD_TYPE}")
message(
  STATUS "  编译器      : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  构建目录    : ${CMAKE_BINARY_DIR}")
message(STATUS "  QML 导入路径: ${DOD_QML_IMPORT_PATH}")
message(STATUS "============================================================")
