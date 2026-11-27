# ==============================================================================
# 定位 Qt5 优先级：-DCMAKE_PREFIX_PATH > QTDIR/QT_ROOT 环境变量 > 常见安装目录 用户自装 :
# ~/Qt5.14.2/5.14.2/gcc_64   (本机就是这种布局) 在线安装 : ~/Qt/5.15.2/gcc_64 /
# ~/Qt/5.15.2/clang_64(macOS) Windows  : C:/Qt/Qt5.15.2/5.15.2/msvc2019_64
# 注意：系统自带的 qtbase5-dev 往往没有 Quick/QuickWidgets 开发包， 所以用户自装的 Qt 目录排在前面，优先被选中。
# ==============================================================================

if(DEFINED ENV{QTDIR})
  list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")
elseif(DEFINED ENV{QT_ROOT})
  list(APPEND CMAKE_PREFIX_PATH "$ENV{QT_ROOT}")
endif()

if(APPLE)
  file(GLOB _qt_guessed_dirs "$ENV{HOME}/Qt5*/*/clang_64"
       "$ENV{HOME}/Qt/5.*/clang_64")
elseif(WIN32)
  if(MSVC)
    file(GLOB _qt_guessed_dirs "C:/Qt/Qt5*/*/msvc*_64" "C:/Qt/5.*/msvc*_64")
  elseif(MINGW)
    file(GLOB _qt_guessed_dirs "C:/Qt/Qt5*/*/mingw*_64" "C:/Qt/5.*/mingw*_64")
  endif()
else()
  file(GLOB _qt_guessed_dirs "$ENV{HOME}/Qt5*/*/gcc_64"
       "$ENV{HOME}/Qt/5.*/gcc_64")
endif()
if(_qt_guessed_dirs)
  list(SORT _qt_guessed_dirs)
  list(REVERSE _qt_guessed_dirs) # 新版本优先
  list(APPEND CMAKE_PREFIX_PATH ${_qt_guessed_dirs})
endif()

find_package(Qt5 5.14 REQUIRED COMPONENTS Core Widgets Quick QuickWidgets Svg
                                          Network)

message(STATUS "已找到 Qt ${Qt5_VERSION}（${Qt5_DIR}）")
