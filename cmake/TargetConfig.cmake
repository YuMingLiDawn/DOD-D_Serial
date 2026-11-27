# ==============================================================================
# 可执行目标：源文件、include 目录、平台属性 依赖：SourceFilesConfig（PROJECT_SOURCES /
# PROJECT_INCLUDE_DIRS）
# ==============================================================================

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE_DIRS})

# 图标资源（svg/png）：从 Qml/res 拷到 exe 同级 res/，方便现场替换图标 （ImageColorizer 优先读磁盘，找不到才回退
# qrc）
#
# 为什么要 OBJECT_DEPENDS：POST_BUILD 属于「链接步骤」，目标没重新链接（源码没变）时 CMake 认为它是最新的，整条
# POST_BUILD 会被跳过 —— 表现就是「改/加了 svg 却不会复制」。 把 res 目录下的文件挂到 main.cpp 的依赖上，任何 svg
# 变化都会让它重编一次 → 重新链接 → POST_BUILD 重新执行。打包目标（create-package）走同一条链，所以包里的 svg 也是新的。
file(GLOB_RECURSE RES_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/Qml/res/*")

if(RES_FILES)
  set_property(
    SOURCE main.cpp
    APPEND
    PROPERTY OBJECT_DEPENDS "${RES_FILES}")
endif()

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
