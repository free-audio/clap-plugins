import QtQuick 2.1
import QtQuick.Layouts

ColumnLayout {
        property int moduleId: 0;
        id: filter

        Knob {
                id: freq
                param: plugin.param(filter.moduleId, 0)
                size: 60 * rootScale
        }

        GridLayout {
                columns: 2

                Knob {
                        id: fm
                        param: plugin.param(filter.moduleId, 5)
                        size: 40 * rootScale
                        Layout.column: 0
                        Layout.row: 0
                }

                Knob {
                        id: reso
                        param: plugin.param(filter.moduleId, 1)
                        size: 40 * rootScale
                        Layout.column: 1
                        Layout.row: 0
                }

                Knob {
                        id: keytrack
                        param: plugin.param(filter.moduleId, 3)
                        size: 40 * rootScale
                        Layout.column: 0
                        Layout.row: 1
                }

                Knob {
                        id: env
                        param: plugin.param(filter.moduleId, 4)
                        size: 40 * rootScale
                        Layout.column: 1
                        Layout.row: 1
                }
        }
}