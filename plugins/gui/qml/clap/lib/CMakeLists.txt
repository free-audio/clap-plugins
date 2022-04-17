qt6_add_qml_module(clap-qml-lib
        URI clap.lib
        VERSION 1.0
        STATIC
        RESOURCE_PREFIX /qml
        QML_FILES
                Knob.qml
                ADSR.qml
                Filter.qml
                DigiOsc.qml
)
