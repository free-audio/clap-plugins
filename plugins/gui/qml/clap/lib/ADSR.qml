import QtQuick 2.1
import QtQuick.Layouts

RowLayout {
        property int moduleId : 0;

        Knob {
                id: attack_knob
                param: plugin.param(moduleId, 0)
                size: 40 * rootScale
        }

        Knob {
                id: decay_knob
                param: plugin.param(moduleId, 1)
                size: 40 * rootScale
        }

        Knob {
                id: sustain_knob
                param: plugin.param(moduleId, 2)
                size: 40 * rootScale
        }

        Knob {
                id: release_knob
                param: plugin.param(moduleId, 3)
                size: 40 * rootScale
        }

        Knob {
                id: velocity_knob
                param: plugin.param(moduleId, 4)
                size: 40 * rootScale
        }
}
