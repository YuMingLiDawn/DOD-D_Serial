# ==============================================================================
# 源文件清单：新增分层目录/文件只改这里
# ==============================================================================

set(PROJECT_SOURCES
    main.cpp
    App/AmsApplication.cpp
    App/AmsApplication.h
    App/ApplicationContext.cpp
    App/ApplicationContext.h
    Model/NicModel/NicModel.cpp
    Model/NicModel/NicModel.h
    Service/SerialService.cpp
    Service/SerialService.h
    Service/NicSearch/NicSearch.cpp
    Service/NicSearch/NicSearch.h
    Infrastructure/NicProvider.cpp
    Infrastructure/NicProvider.h
    Infrastructure/Interface/INicProvider.h
    Utility/Log.cpp
    Utility/Log.h
    Utility/ImageColorizer/ImageColorizer.h
    Utility/ImageColorizer/ImageColorizer.cpp
    Qml/Qml.qrc)

# 作为跨层 include 根目录，业务里依然写 #include "SerialService.h"
set(PROJECT_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/App ${PROJECT_SOURCE_DIR}/Model
    ${PROJECT_SOURCE_DIR}/Service ${PROJECT_SOURCE_DIR}/Infrastructure
    ${PROJECT_SOURCE_DIR}/Utility)
