include(D:/DeskPet/DeskPet/build/Desktop_Qt_6_7_3_MinGW_64_bit-Debug/.qt/QtDeploySupport.cmake)
include("${CMAKE_CURRENT_LIST_DIR}/DeskPet-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_ALL_MODULES_FOUND_VIA_FIND_PACKAGE "ZlibPrivate;EntryPointPrivate;Core;Gui;Widgets")

qt6_deploy_runtime_dependencies(
    EXECUTABLE D:/DeskPet/DeskPet/build/Desktop_Qt_6_7_3_MinGW_64_bit-Debug/DeskPet.exe
    GENERATE_QT_CONF
)
