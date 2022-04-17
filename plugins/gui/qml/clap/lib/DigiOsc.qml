import QtQuick 2.1
import QtQuick.Layouts

RowLayout {
        property int moduleId : 0;

        Knob {
                id: tuning
                param: plugin.param(moduleId, 0)
                size: 40 * rootScale
        }
}
