# ==============================================================================
# QWidget + QQuickWidget + QML 空白窗口（Qt 5 / CMake / 多平台） 结构：基础配置 -> 定位 Qt -> 目标定义
# -> 平台属性 -> QML 部署 -> 便捷目标
# ==============================================================================
cmake_minimum_required(VERSION 3.19)

project(
  DOD-D_Serial
  VERSION 0.1.0
  LANGUAGES CXX)

# 可执行文件统一输出到 <build>/bin（多配置生成器自动再加 /Release 等子目录） 好处：打包时只需 zip 这个目录，不会把
# CMakeCache/Makefile 混进安装包
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Qt5 没有 qt_standard_project_setup()，MOC/UIC/RCC 手动打开
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ------------------------------------------------------------------------------
# 定位 Qt5，优先级：-DCMAKE_PREFIX_PATH > QTDIR/QT_ROOT 环境变量 > 常见安装目录 用户自装 :
# ~/Qt5.14.2/5.14.2/gcc_64   (本机就是这种布局) 在线安装 : ~/Qt/5.15.2/gcc_64 /
# ~/Qt/5.15.2/clang_64(macOS) Windows  : C:/Qt/Qt5.15.2/5.15.2/msvc2019_64
# 注意：系统自带的 qtbase5-dev 往往没有 Quick/QuickWidgets 开发包，所以用户自装的 Qt 目录排在前面，优先被选中。
# ------------------------------------------------------------------------------
if(DEFINED ENV{QTDIR})
  list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")
elseif(DEFINED ENV{QT_ROOT})
  list(APPEND CMAKE_PREFIX_PATH "$ENV{QT_ROOT}")
endif()

if(APPLE)
  file(GLOB _qt_guessed_dirs "$ENV{HOME}/Qt5*/*/clang_64"
       "$ENV{HOME}/Qt/5.*/clang_64")
elseif(WIN32)
  file(GLOB _qt_guessed_dirs "C:/Qt/Qt5*/*/msvc*_64" "C:/Qt/5.*/msvc*_64")
else()
  file(GLOB _qt_guessed_dirs "$ENV{HOME}/Qt5*/*/gcc_64"
       "$ENV{HOME}/Qt/5.*/gcc_64")
endif()
if(_qt_guessed_dirs)
  list(SORT _qt_guessed_dirs)
  list(REVERSE _qt_guessed_dirs) # 新版本优先
  list(APPEND CMAKE_PREFIX_PATH ${_qt_guessed_dirs})
endif()

find_package(Qt5 5.12 REQUIRED COMPONENTS Core Widgets Quick QuickWidgets)

message(STATUS "已找到 Qt ${Qt5_VERSION}（${Qt5_DIR}）")

# ------------------------------------------------------------------------------
# 第三方依赖：spdlog（日志） 优先级：仓库内置 third_party/spdlog > 系统安装包 > 在线拉取 - 内置版：别人 clone
# 下来无需联网、无需额外安装，直接能编译 - 换日志库时：删掉这段 + 重写 Utility/Log.cpp（业务代码零改动）
# ------------------------------------------------------------------------------
set(_spdlog_vendor "${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog")
set(DOD_SPDLOG_TARGET "")
find_package(Threads REQUIRED)

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

# ------------------------------------------------------------------------------
# 可执行目标
# ------------------------------------------------------------------------------
add_executable(
  ${PROJECT_NAME}
  main.cpp
  App/AmsApplication.cpp
  App/AmsApplication.h
  App/ApplicationContext.cpp
  App/ApplicationContext.h
  Service/SerialService.cpp
  Service/SerialService.h
  Model/SerialPortModel.cpp
  Model/SerialPortModel.h
  Service/NicSearch/NicSearch.cpp
  Service/NicSearch/NicSearch.h
  Utility/Log.cpp
  Utility/Log.h
  Qml/Qml.qrc)

# 分层目录做成公共 include 目录，跨层引用依然写 #include "SerialService.h"
target_include_directories(
  ${PROJECT_NAME}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/App ${CMAKE_CURRENT_SOURCE_DIR}/Model
          ${CMAKE_CURRENT_SOURCE_DIR}/Service
          ${CMAKE_CURRENT_SOURCE_DIR}/Utility)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE Qt5::Core
          Qt5::Widgets
          Qt5::Quick
          Qt5::QuickWidgets
          ${DOD_SPDLOG_TARGET}
          # spdlog 用到 std::thread：Linux 需显式链接 pthread（Windows/macOS 上为空）
          Threads::Threads)

# ------------------------------------------------------------------------------
# 平台属性 macOS：打成 .app（applicationDirPath() = xxx.app/Contents/MacOS，QML 就放这里）
# Windows：默认 GUI 子系统（无控制台），qDebug 走调试器输出； 想在终端里直接看 qDebug 就配置时加
# -DDOD_WIN32_CONSOLE=ON
# ------------------------------------------------------------------------------
set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)

if(WIN32)
  # Qt5 的 add_executable 默认是控制台程序，这里显式切成 GUI
  set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
  option(DOD_WIN32_CONSOLE "Windows 下保留控制台窗口，便于查看 qDebug 输出" OFF)
  if(DOD_WIN32_CONSOLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE FALSE)
  endif()
endif()

# ------------------------------------------------------------------------------
# QML 用 qrc 资源方式编进可执行文件（Qml/Qml.qrc 由 AUTORCC 自动 rcc 编译） - 资源路径
# qrc:/Qml/Main.qml，exe 走到哪都能加载，不依赖外部目录 - 改 QML 后重新构建即可，AUTORCC 会自动检测 Qml/*.qml
# 的时间戳 若想改成「外部文件」方式，删掉 Qml.qrc 并改用 applicationDirPath() + "/Qml/Main.qml" 加载
# ------------------------------------------------------------------------------

# 只改 QML 时也要触发一次重建，保证 rcc 重新打包资源
file(GLOB_RECURSE QML_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/Qml/*.qml"
     "${CMAKE_CURRENT_SOURCE_DIR}/Qml/Qml.qrc")
if(QML_FILES)
  set_property(
    SOURCE main.cpp
    APPEND
    PROPERTY OBJECT_DEPENDS "${QML_FILES}")
endif()

# ------------------------------------------------------------------------------
# 便捷目标（三平台通用）：cmake --build <build-dir> --target run
# ------------------------------------------------------------------------------
add_custom_target(
  run
  COMMAND $<TARGET_FILE:${PROJECT_NAME}>
  DEPENDS ${PROJECT_NAME}
  USES_TERMINAL
  COMMENT "启动 ${PROJECT_NAME}")

# ------------------------------------------------------------------------------
# Qt 运行库部署 + 打包（参考 DOD-WTC3 的 Windows 打包方式） Windows : windeployqt 把 Qt
# DLL/插件/QML 模块摆到 exe 旁边，再打 zip macOS   : macdeployqt 把 Qt 框架内嵌进 .app，再打 zip
# Linux   : 依赖系统 Qt，只把可执行文件 + qml/ 打 zip 用法：cmake --build <build-dir> --config
# Release --target create-package
# ------------------------------------------------------------------------------
get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)

# 平台标识，用进包名，例如 DOD-D_Serial-0.1.0-Linux-x86_64.zip
if(WIN32)
  set(_pkg_os "Windows")
elseif(APPLE)
  set(_pkg_os "macOS")
else()
  set(_pkg_os "${CMAKE_SYSTEM_NAME}")
endif()
set(PACKAGE_NAME
    "${PROJECT_NAME}-${PROJECT_VERSION}-${_pkg_os}-${CMAKE_SYSTEM_PROCESSOR}")
set(PACKAGE_ZIP "${CMAKE_BINARY_DIR}/${PACKAGE_NAME}.zip")

if(WIN32)
  find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")
  if(WINDEPLOYQT_EXECUTABLE)
    # 每次构建后自动把 Qt 运行库部署到 exe 同级目录
    add_custom_command(
      TARGET ${PROJECT_NAME}
      POST_BUILD
      COMMAND
        "${WINDEPLOYQT_EXECUTABLE}"
        $<$<CONFIG:Debug>:--debug>$<$<NOT:$<CONFIG:Debug>>:--release> --qmldir
        "${CMAKE_CURRENT_SOURCE_DIR}/Qml" --dir
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>" "$<TARGET_FILE:${PROJECT_NAME}>"
      COMMENT "windeployqt：部署 Qt 运行库到 $<TARGET_FILE_DIR:${PROJECT_NAME}>"
      VERBATIM)
    message(STATUS "Windows: 已找到 windeployqt，构建后自动部署 Qt 运行库")
  else()
    message(WARNING "未找到 windeployqt（Qt 安装的 bin 目录：${QT_BIN_DIR}）")
  endif()

  add_custom_target(
    deploy-qt
    DEPENDS ${PROJECT_NAME}
    COMMENT "Qt 运行库已在构建时由 windeployqt 部署")

  # zip 根目录：可执行文件所在目录（exe + qml + Qt DLL 全在里面）
  set(_pkg_src_dir "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
  set(_pkg_zip_item ".")

elseif(APPLE)
  find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${QT_BIN_DIR}")
  if(MACDEPLOYQT_EXECUTABLE)
    add_custom_target(
      deploy-qt
      COMMAND "${MACDEPLOYQT_EXECUTABLE}" "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
              -qmldir="${CMAKE_CURRENT_SOURCE_DIR}/Qml" -always-overwrite
      DEPENDS ${PROJECT_NAME}
      USES_TERMINAL
      COMMENT "macdeployqt：内嵌 Qt 框架到 .app")
  else()
    message(WARNING "未找到 macdeployqt（Qt 安装的 bin 目录：${QT_BIN_DIR}）")
    add_custom_target(deploy-qt DEPENDS ${PROJECT_NAME})
  endif()

  # zip 根目录：.app 所在目录，包内保留完整的 xxx.app
  set(_pkg_src_dir "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../..")
  set(_pkg_zip_item "${PROJECT_NAME}.app")

else()
  add_custom_target(
    deploy-qt
    DEPENDS ${PROJECT_NAME}
    COMMENT "Linux：使用系统 Qt 库，无需部署")

  set(_pkg_src_dir "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
  set(_pkg_zip_item ".")
endif()

add_custom_target(
  create-package
  COMMAND ${CMAKE_COMMAND} -E rm -f "${PACKAGE_ZIP}"
  # zip 格式自带 deflate，不能再加 z（gzip）压缩标志，否则 CMake 报
  # "Can not use compression flags with format: zip"
  COMMAND ${CMAKE_COMMAND} -E tar cf "${PACKAGE_ZIP}" --format=zip --
          ${_pkg_zip_item}
  COMMAND ${CMAKE_COMMAND} -E echo "打包完成: ${PACKAGE_ZIP}"
  WORKING_DIRECTORY "${_pkg_src_dir}"
  DEPENDS deploy-qt
  USES_TERMINAL
  COMMENT "生成 zip 安装包"
  VERBATIM)

# 别名，兼容 WTC 习惯的叫法
add_custom_target(create-zip DEPENDS create-package)

message(STATUS "------------------------------------------------------------")
message(STATUS "  项目        : ${PROJECT_NAME}")
message(STATUS "  Qt 前缀     : ${CMAKE_PREFIX_PATH}")
message(STATUS "  构建类型    : ${CMAKE_BUILD_TYPE}")
message(STATUS "  构建目录    : ${CMAKE_BINARY_DIR}")
message(STATUS "------------------------------------------------------------")
