cmake_minimum_required(VERSION 3.19)
project(DeskPet LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools)

qt_standard_project_setup()

qt_add_executable(DeskPet
    WIN32 MACOSX_BUNDLE
    main.cpp




    PetWindow.h PetWindow.cpp
    animationmanager.h animationmanager.cpp
    petcontroller.h petcontroller.cpp
    traymenumanager.h traymenumanager.cpp
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
)

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
