# ==============================================================================
# 项目基础配置：输出目录、C++ 标准、Qt 构建工具开关 只放"跟项目本身相关的通用设置"，不涉及任何依赖查找
# ==============================================================================

# 可执行文件统一输出到 <build>/bin（多配置生成器自动再加 /Release 等子目录） 好处：打包时只需 zip 这个目录，不会把
# CMakeCache/Makefile 混进安装包
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt5 没有 qt_standard_project_setup()，MOC/UIC/RCC 手动打开
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
