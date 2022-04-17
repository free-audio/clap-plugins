import QtQuick 2.1
import QtQuick.Layouts

GridLayout {
        property int moduleId: 0;
        columns: 3

        Knob {
                id: freq
                param: plugin.param(moduleId, 0)
                size: 60 * rootScale
                Layout.column: 1
                Layout.row: 0
        }

        Knob {
                id: reso
                param: plugin.param(moduleId, 1)
                size: 40 * rootScale
                Layout.column: 2
                Layout.row: 0
        }

        Knob {
                id: keytrack
                param: plugin.param(moduleId, 3)
                size: 40 * rootScale
                Layout.column: 0
                Layout.row: 1
        }

        Knob {
                id: env
                param: plugin.param(moduleId, 4)
                size: 40 * rootScale
                Layout.column: 1
                Layout.row: 1
        }

        Knob {
                id: fm
                param: plugin.param(moduleId, 5)
                size: 40 * rootScale
                Layout.column: 2
                Layout.row: 1
        }
}
