import QtQuick 2.1
import QtQuick.Controls 2.1
import QtQuick.Layouts
import clap.lib

Rectangle {
        width: 1000 * rootScale
        height: 600 * rootScale
        color: "#224477"

        RowLayout {
                ColumnLayout {
                        spacing: 10

                        GroupBox {
                                title: "Osc1"
                                DigiOsc {
                                        id: osc1
                                        moduleId: 4
                                }
                        }

                        GroupBox {
                                title: "Osc2"
                                DigiOsc {
                                        id: osc2
                                        moduleId: 5
                                }
                        }
                }
                ColumnLayout {
                        spacing: 10

                        GroupBox {
                                title: "Filter"
                                Filter {
                                        id: filter
                                        moduleId: 3
                                }
                        }
                }
                ColumnLayout {
                        spacing: 10

                        GroupBox {
                                title: "FEG"
                                ADSR {
                                        id: filter_adsr
                                        moduleId: 2
                                }
                        }

                        GroupBox {
                                title: "AEG"
                                ADSR {
                                        id: amp_adsr
                                        moduleId: 6
                                }
                        }
                }
        }
}
