import QtQuick 2.1
import QtQuick.Controls 2.1
import clap.lib

Rectangle {
    width: 200 * rootScale
    height: 200 * rootScale
    color: "#224477"

    Knob {
        id: dc_offset_knob
        param: plugin.param(0)
        size: 160 * rootScale
        x: 20 * rootScale
        y: 20 * rootScale
    }
}
