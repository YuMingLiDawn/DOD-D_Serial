# ==============================================================================
# DOD-D_Serial 主 CMakeLists（Qt5 / 多平台）
#
# 这里只做「入口 + 顺序编排」，具体配置都在 cmake/ 下的子文件里：
#   1. ProjectConfig       输出目录 / C++17 / MOC-UIC-RCC
#   2. SourceFilesConfig   源文件与 include 目录清单（加文件只改这里）
#   3. Qt5Config           定位 Qt5（QTDIR / 常见安装目录）
#   4. ThirdPartyConfig    第三方依赖（spdlog / Threads）
#   5. TargetConfig        可执行目标 + 平台属性
#   6. LinkLibrariesConfig 链接依赖
#   7. QmlConfig           QML(qrc) 资源依赖
#   8. PackageConfig       run / windeployqt / create-package
#   9. Summary             配置结论输出
# ==============================================================================
cmake_minimum_required(VERSION 3.19)

project(
  DOD-D_Serial
  VERSION 0.1.0
  LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(ProjectConfig)
include(SourceFilesConfig)
include(Qt5Config)
include(ThirdPartyConfig)
include(TargetConfig)
include(LinkLibrariesConfig)
include(QmlConfig)
include(PackageConfig)
include(Summary)
