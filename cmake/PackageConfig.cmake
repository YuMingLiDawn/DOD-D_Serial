# ==============================================================================
# 运行与打包 run            : cmake --build <build> --target run（三平台通用启动） deploy-qt :
# Windows=windeployqt，macOS=macdeployqt，Linux=无需 create-package : 把可执行目录打成
# zip（Windows 包里带 Qt DLL/插件） create-zip     : create-package 的别名（兼容 WTC 习惯叫法）
# 依赖：TargetConfig（目标已创建）、Qt5Config（Qt5::qmake 用于定位 Qt bin）
# ==============================================================================

# ---------------------------------------------------------------- run
add_custom_target(
  run
  COMMAND $<TARGET_FILE:${PROJECT_NAME}>
  DEPENDS ${PROJECT_NAME}
  USES_TERMINAL
  COMMENT "启动 ${PROJECT_NAME}")

# ---------------------------------------------------------------- 包名
get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)

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

# ---------------------------------------------------------------- 各平台部署
if(WIN32)
  set(WINDEPLOYQT_EXECUTABLE "${QT_BIN_DIR}/windeployqt.exe")
  if(NOT EXISTS "${WINDEPLOYQT_EXECUTABLE}")
    message(FATAL_ERROR "未找到与所选 Qt 配套的 windeployqt（${QT_BIN_DIR}）")
  endif()

  set(_windeployqt_args
      $<$<CONFIG:Debug>:--debug>$<$<NOT:$<CONFIG:Debug>>:--release>
      --qmldir "${PROJECT_SOURCE_DIR}/Qml"
      --dir "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
      "$<TARGET_FILE:${PROJECT_NAME}>")
  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND "${WINDEPLOYQT_EXECUTABLE}" ${_windeployqt_args}
    COMMENT "windeployqt：部署 Qt 运行库到 $<TARGET_FILE_DIR:${PROJECT_NAME}>"
    VERBATIM)

  add_custom_target(
    deploy-qt
    COMMAND "${WINDEPLOYQT_EXECUTABLE}" ${_windeployqt_args}
    DEPENDS ${PROJECT_NAME}
    COMMENT "windeployqt：部署 Qt 运行库"
    VERBATIM)

  set(_pkg_src_dir "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
  set(_pkg_zip_item ".")

  install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
  install(DIRECTORY "${PROJECT_SOURCE_DIR}/Qml/res/" DESTINATION bin/res)
  install(CODE "
    if(CMAKE_INSTALL_CONFIG_NAME STREQUAL \"Debug\")
      set(deploy_mode --debug)
    elseif(CMAKE_INSTALL_CONFIG_NAME STREQUAL \"Release\")
      set(deploy_mode --release)
    else()
      message(FATAL_ERROR \"Windows 安装包仅支持 Debug 或 Release 构建\")
    endif()
    execute_process(
      COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" \${deploy_mode} --no-compiler-runtime
              --qmldir \"${PROJECT_SOURCE_DIR}/Qml\"
              --dir \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin\"
              \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin/$<TARGET_FILE_NAME:${PROJECT_NAME}>\"
      RESULT_VARIABLE deploy_result)
    if(NOT deploy_result EQUAL 0)
      message(FATAL_ERROR \"windeployqt 失败: \${deploy_result}\")
    endif()
  ")

  if(MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    include(InstallRequiredSystemLibraries)
    if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
      install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
              DESTINATION bin CONFIGURATIONS Release)
    else()
      install(CODE "
        if(CMAKE_INSTALL_CONFIG_NAME STREQUAL \"Release\")
          message(FATAL_ERROR \"找不到 MSVC 可再分发运行库，请安装对应的 Visual C++ 工具集\")
        endif()
      ")
    endif()
  else()
    install(CODE "message(FATAL_ERROR \"Windows 安装包目前只支持 MSVC 构建\")")
  endif()

  set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
  set(CPACK_PACKAGE_VENDOR "${PROJECT_NAME}")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
  set(CPACK_PACKAGE_FILE_NAME "${PACKAGE_NAME}-Setup")
  set(CPACK_PACKAGE_EXECUTABLES "${PROJECT_NAME}" "${PROJECT_NAME}")
  include(CPack)

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_installer_generator NSIS64)
  else()
    set(_installer_generator NSIS)
  endif()
  add_custom_target(
    create-installer
    COMMAND "${CMAKE_CPACK_COMMAND}" -G "${_installer_generator}" -C "$<CONFIG>"
            --config "${CMAKE_BINARY_DIR}/CPackConfig.cmake"
            -D "CPACK_PACKAGE_FILE_NAME=${PACKAGE_NAME}-$<CONFIG>-Setup"
            -D "CPACK_PACKAGE_INSTALL_DIRECTORY=${PROJECT_NAME}$<$<CONFIG:Debug>:-Debug>"
            -D "CPACK_PACKAGE_INSTALL_REGISTRY_KEY=${PROJECT_NAME}$<$<CONFIG:Debug>:-Debug>"
            -D "CPACK_NSIS_DISPLAY_NAME=${PROJECT_NAME} $<CONFIG>"
            -D "CPACK_NSIS_PACKAGE_NAME=${PROJECT_NAME} $<CONFIG>"
            -B "${CMAKE_BINARY_DIR}"
    DEPENDS ${PROJECT_NAME}
    USES_TERMINAL
    COMMENT "生成 Windows $<CONFIG> 安装程序"
    VERBATIM)

elseif(APPLE)
  find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${QT_BIN_DIR}")
  if(MACDEPLOYQT_EXECUTABLE)
    add_custom_target(
      deploy-qt
      COMMAND "${MACDEPLOYQT_EXECUTABLE}" "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
              -qmldir="${PROJECT_SOURCE_DIR}/Qml" -always-overwrite
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

# ---------------------------------------------------------------- 打 zip
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

add_custom_target(create-zip DEPENDS create-package)
