import QtQuick 2.1
import QtQuick.Controls 2.1
import QtQuick.Layouts
import clap.lib

Rectangle {
        width: 600 * rootScale
        height: 300 * rootScale
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
                GroupBox {
                        title: "Mixer"
                        ColumnLayout {
                                Knob {
                                        id: osc1Gain
                                        param: plugin.param(7, 0)
                                        size: 40 * rootScale
                                }

                                Knob {
                                        id: osc2Gain
                                        param: plugin.param(7, 1)
                                        size: 40 * rootScale
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

                Knob {
                        id: volume
                        param: plugin.param(7, 2)
                        size: 40 * rootScale
                }
        }
}
