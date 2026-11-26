# ==============================================================================
# 链接配置：Qt 模块 + 第三方库
# 依赖：Qt5Config（Qt5::*）、ThirdPartyConfig（DOD_SPDLOG_TARGET）、TargetConfig（目标已创建）
# ==============================================================================

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE Qt5::Core
          Qt5::Widgets
          Qt5::Quick
          Qt5::QuickWidgets
          Qt5::Svg # ImageColorizer 渲染/着色 SVG 用
          ${DOD_SPDLOG_TARGET}
          Threads::Threads)
