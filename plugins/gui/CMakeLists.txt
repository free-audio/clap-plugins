set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6Core CONFIG REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)
find_package(Qt6 COMPONENTS Qml Quick QuickControls2 REQUIRED)

add_library(clap-plugin-gui
application.hh
application.cc

parameter-proxy.hh
parameter-proxy.cc
plugin-proxy.hh
plugin-proxy.cc
transport-proxy.hh
transport-proxy.cc
)

target_link_libraries(clap-plugin-gui PUBLIC clap-io Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Widgets Qt6::Core )
qt6_import_qml_plugins(clap-plugin-gui)
set_target_properties(clap-plugin-gui PROPERTIES
    POSITION_INDEPENDENT_CODE true
    PROPERTIES CXX_STANDARD 17)
if(false)
    # Static link of qml gui
    qt6_add_qml_module(clap-plugin-gui
        URI clap
        VERSION 1.0
        RESOURCE_PREFIX /clap/imports
        QML_FILES
            qml/clap/Knob.qml
    )
endif()

add_executable(clap-gui main.cc)
target_link_libraries(clap-gui PRIVATE clap-plugin-gui)
set_target_properties(clap-gui PROPERTIES CXX_STANDARD 17)
install(TARGETS clap-gui DESTINATION "bin")
