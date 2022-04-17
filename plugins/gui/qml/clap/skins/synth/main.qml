import QtQuick 2.1
import QtQuick.Controls 2.1
import QtQuick.Layouts
import clap.lib

Rectangle {
        width: 500 * rootScale
        height: 300 * rootScale
        color: "#224477"

        ColumnLayout {
                spacing: 10

                ADSR {
                        id: filter_adsr
                        moduleId: 2
                }

                ADSR {
                        id: amp_adsr
                        moduleId: 6
                }
        }
}
