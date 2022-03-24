qt6_add_qml_module(clap-qml-skins
        URI clap.skins
        VERSION 1.0
        STATIC
        RESOURCE_PREFIX /qml
        QML_FILES
                dc-offset/main.qml
                gain/main.qml
                transport-info/main.qml
                char-check/main.qml
)
