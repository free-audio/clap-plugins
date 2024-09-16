import QtQuick 6.0
import QtQuick.Controls 6.0
import clap.lib

Rectangle {
    width: 200
    height: 200
    color: "#224477"

    Knob {
        id: gain_knob
        param: plugin.param(0)
        size: 160
        x: 20
        y: 20
    }
}
