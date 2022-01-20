set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

#find_package(Qt6 COMPONENTS Core Widget Platform Qml Quick QuickControls2 REQUIRED)

add_library(clap-plugin-gui-common
    abstract-gui.hh
    abstract-gui-listener.hh
    abstract-gui-listener.cc
    abstract-gui-client-factory.hh)
set_property(TARGET clap-plugin-gui-common PROPERTY CXX_STANDARD 20)
set_property(TARGET clap-plugin-gui-common PROPERTY POSITION_INDEPENDENT_CODE true)
target_link_libraries(clap-plugin-gui-common PUBLIC clap-core)

add_library(clap-plugin-gui
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
target_link_libraries(clap-plugin-gui PUBLIC clap-io clap-plugin-gui-common)
target_link_libraries(clap-plugin-gui PRIVATE Qt6::QuickControls2 Qt6::Quick Qt6::Qml Qt6::Widgets Qt6::Core)
set_property(TARGET clap-plugin-gui PROPERTY POSITION_INDEPENDENT_CODE true)
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

add_library(clap-plugin-local-gui
    local-gui-client-factory.hh
    local-gui-client-factory.cc
    local-gui-proxy.hh
    local-gui-proxy.cc)
set_property(TARGET clap-plugin-local-gui PROPERTY CXX_STANDARD 20)
set_property(TARGET clap-plugin-local-gui PROPERTY POSITION_INDEPENDENT_CODE true)
target_link_libraries(clap-plugin-local-gui PUBLIC clap-plugin-gui)

add_library(clap-plugin-remote-gui
    remote-gui-client-factory-proxy.hh
    remote-gui-client-factory-proxy.cc)
set_property(TARGET clap-plugin-remote-gui PROPERTY CXX_STANDARD 20)
target_link_libraries(clap-plugin-remote-gui PUBLIC clap-plugin-gui-common clap-io)

add_executable(clap-gui
    main.cc
    remote-gui-client-factory.hh
    remote-gui-client-factory.cc)
target_link_libraries(clap-gui PUBLIC clap-plugin-gui)
set_target_properties(clap-gui PROPERTIES CXX_STANDARD 20)

if (use_remote_gui)
    install(TARGETS clap-gui DESTINATION "bin")
endif()
