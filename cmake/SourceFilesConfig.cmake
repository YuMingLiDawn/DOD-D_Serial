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
    Service/NicSearch/NicSearch.cpp
    Service/NicSearch/NicSearch.h
    Service/UdpReceiver/UdpReceiver.cpp
    Service/UdpReceiver/UdpReceiver.h
    Infrastructure/NicProvider.cpp
    Infrastructure/NicProvider.h
    Infrastructure/UdpTransportQt.cpp
    Infrastructure/UdpTransportQt.h
    Infrastructure/Interface/INicProvider.h
    Infrastructure/Interface/IUdpTransport.h
    Utility/Log.cpp
    Utility/Log.h
    Utility/ImageColorizer/ImageColorizer.h
    Utility/ImageColorizer/ImageColorizer.cpp
    Protocol/Ulxd/UlxdTypes.cpp
    Protocol/Ulxd/UlxdTypes.h
    Protocol/Ulxd/UlxdMessage.cpp
    Protocol/Ulxd/UlxdMessage.h
    Protocol/Ulxd/UlxdFrameReader.h
    Protocol/Ulxd/UlxdCommands.cpp
    Protocol/Ulxd/UlxdCommands.h
    Protocol/Ulxd/UlxdClient.cpp
    Protocol/Ulxd/UlxdClient.h
    Qml/Qml.qrc)

# 作为跨层 include 根目录，业务里依然写 #include "SerialService.h"
set(PROJECT_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/App
    ${PROJECT_SOURCE_DIR}/Model
    ${PROJECT_SOURCE_DIR}/Service
    ${PROJECT_SOURCE_DIR}/Infrastructure
    ${PROJECT_SOURCE_DIR}/Utility
    ${PROJECT_SOURCE_DIR}/Protocol)
