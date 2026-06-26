cmake_minimum_required(VERSION 3.19)
project(DeskPet LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Required by cpp-httplib on Windows (must be set before any Windows header)
add_compile_definitions(_WIN32_WINNT=0x0A00)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network LinguistTools)

qt_standard_project_setup()

# ── cpp-httplib (header-only) ──────────────────────────────
add_library(httplib INTERFACE)
target_include_directories(httplib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/cpp-httplib-0.48.0)

# ── libqrencode (static, core encoding only) ───────────────
set(QRENCODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libqrencode-4.1.1)
add_library(qrencode STATIC
    ${QRENCODE_DIR}/qrencode.c
    ${QRENCODE_DIR}/qrinput.c
    ${QRENCODE_DIR}/bitstream.c
    ${QRENCODE_DIR}/qrspec.c
    ${QRENCODE_DIR}/rsecc.c
    ${QRENCODE_DIR}/split.c
    ${QRENCODE_DIR}/mask.c
    ${QRENCODE_DIR}/mqrspec.c
    ${QRENCODE_DIR}/mmask.c
)
target_include_directories(qrencode PUBLIC ${QRENCODE_DIR})
target_compile_definitions(qrencode PRIVATE
    STATIC_IN_RELEASE=static HAVE_CONFIG_H=0
    MAJOR_VERSION=4 MINOR_VERSION=1 MICRO_VERSION=1
    VERSION=\"4.1.1\")
if(MSVC)
    target_compile_definitions(qrencode PRIVATE
        strcasecmp=_stricmp strncasecmp=_strnicmp
        _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
endif()

# ── Executable ─────────────────────────────────────────────
qt_add_executable(DeskPet
    WIN32 MACOSX_BUNDLE
    main.cpp

    PetWindow.h PetWindow.cpp
    animationmanager.h animationmanager.cpp
    petcontroller.h petcontroller.cpp
    traymenumanager.h traymenumanager.cpp
    aichatmanager.h aichatmanager.cpp
    aisettingsdialog.h aisettingsdialog.cpp
    chatbubbledialog.h chatbubbledialog.cpp
    FileTransferServer.h FileTransferServer.cpp
    FileTransferDialog.h FileTransferDialog.cpp
    QRCodeGenerator.h QRCodeGenerator.cpp
    res.qrc
)

qt_add_translations(
    TARGETS DeskPet
    TS_FILES DeskPet_zh_CN.ts
)

target_link_libraries(DeskPet
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        httplib
        qrencode
)

if(WIN32)
    target_link_libraries(DeskPet PRIVATE ws2_32 crypt32)
endif()

install(TARGETS DeskPet
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET DeskPet
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
