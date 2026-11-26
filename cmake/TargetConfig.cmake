# ==============================================================================
# 可执行目标：源文件、include 目录、平台属性 依赖：SourceFilesConfig（PROJECT_SOURCES /
# PROJECT_INCLUDE_DIRS）
# ==============================================================================

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE_DIRS})

# 图标资源（svg/png）： 1) 已作为 Qml/res/* 登记进 qrc → 编进 exe，随 exe 一起被打包； 2) 这里再拷一份到 exe
# 同级 res/，方便现场替换图标（ImageColorizer 优先读磁盘）。
add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/Qml/res"
          "$<TARGET_FILE_DIR:${PROJECT_NAME}>/res"
  COMMENT "复制图标资源到 $<TARGET_FILE_DIR:${PROJECT_NAME}>/res"
  VERBATIM)

# macOS：打成 .app（applicationDirPath() = xxx.app/Contents/MacOS，QML 已内嵌，不受影响）
set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)

if(WIN32)
  # Qt5 的 add_executable 默认是控制台程序，这里显式切成 GUI
  set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
  option(DOD_WIN32_CONSOLE "Windows 下保留控制台窗口，便于查看日志输出" OFF)
  if(DOD_WIN32_CONSOLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE FALSE)
  endif()
endif()
