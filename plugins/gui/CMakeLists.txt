set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6Core CONFIG REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)
find_package(Qt6 COMPONENTS Qml Quick QuickControls2 REQUIRED)

add_library(clap-plugin-gui
abstract-gui.hh
abstract-gui-listener.hh

abstract-gui-client-factory.hh
remote-gui-client-factory.hh
remote-gui-client-factory.cc
remote-gui-client-factory-proxy.hh
remote-gui-client-factory-proxy.cc
local-gui-client-factory.hh
local-gui-client-factory.cc

local-gui-proxy.hh
local-gui-proxy.cc

gui-client-factory.cc
gui-client.hh
gui-client.cc

parameter-proxy.hh
parameter-proxy.cc
plugin-proxy.hh
plugin-proxy.cc
transport-proxy.hh
transport-proxy.cc
)

set_property(TARGET clap-plugin-gui PROPERTY CXX_STANDARD 20)
if(use_local_gui)
    set_property(TARGET clap-plugin-gui PROPERTYPOSITION_INDEPENDENT_CODE true)
endif()

target_link_libraries(clap-plugin-gui PUBLIC clap-io Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Widgets Qt6::Core )
qt6_import_qml_plugins(clap-plugin-gui)

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

if (use_remote_gui)
    add_executable(clap-gui main.cc)
    target_link_libraries(clap-gui PUBLIC clap-plugin-gui)
    set_target_properties(clap-gui PROPERTIES CXX_STANDARD 20)
    install(TARGETS clap-gui DESTINATION "bin")
endif()