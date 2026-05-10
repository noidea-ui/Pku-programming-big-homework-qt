include(E:/Games/GreatProject/Pku-programming-big-homework-qt/DeskPet/build/.qt/QtDeploySupport.cmake)
include("${CMAKE_CURRENT_LIST_DIR}/DeskPet-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_ALL_MODULES_FOUND_VIA_FIND_PACKAGE "ZlibPrivate;EntryPointPrivate;Core;Gui;Widgets")

qt6_deploy_runtime_dependencies(
    EXECUTABLE E:/Games/GreatProject/Pku-programming-big-homework-qt/DeskPet/build/DeskPet.exe
    GENERATE_QT_CONF
)
