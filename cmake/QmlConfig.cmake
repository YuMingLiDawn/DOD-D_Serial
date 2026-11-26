# ==============================================================================
# QML 资源（qrc）配置 - Qml/*.qml 与 Qml/Qml.qrc 由 AUTORCC 自动 rcc 编译进 exe - 资源路径
# qrc:/Main.qml，exe 走到哪都能加载，不依赖外部目录 - 只改 QML 时也要触发一次重建，否则 POST_BUILD/资源不会刷新
# ==============================================================================

file(GLOB_RECURSE QML_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/Qml/*.qml"
     "${PROJECT_SOURCE_DIR}/Qml/Qml.qrc")

if(QML_FILES)
  set_property(
    SOURCE main.cpp
    APPEND
    PROPERTY OBJECT_DEPENDS "${QML_FILES}")
endif()

# 目录导入（import "Common"）等价于运行时把 Qml/ 加到模块搜索路径； 命名模块（import Common 1.0）依赖下面这行把 qrc
# 根加入 import path。 这里只记录，实际 addImportPath 在 App/AmsApplication.cpp 里对每个引擎设置。
set(DOD_QML_IMPORT_PATH "qrc:/")
