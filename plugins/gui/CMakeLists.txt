set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6Core CONFIG REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)
find_package(Qt6 COMPONENTS Qml Quick QuickControls2 REQUIRED)

add_executable(clap-gui
    main.cc

    application.hh
    application.cc

    parameter-proxy.hh
    parameter-proxy.cc
    plugin-proxy.hh
    plugin-proxy.cc
    transport-proxy.hh
    transport-proxy.cc
)
target_link_libraries(clap-gui PRIVATE clap-io Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Widgets Qt6::Core )
qt6_import_qml_plugins(clap-gui)

if(false)
    # Static link of qml gui
    qt6_add_qml_module(clap-gui
        URI clap
        VERSION 1.0
        RESOURCE_PREFIX /clap/imports
        QML_FILES
            qml/clap/Knob.qml
    )
endif()

set_target_properties(clap-gui PROPERTIES CXX_STANDARD 17)
install(TARGETS clap-gui DESTINATION "bin")
