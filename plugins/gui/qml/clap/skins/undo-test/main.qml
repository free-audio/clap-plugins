import QtQuick 6.7
import QtQuick.Controls 6.7
import QtQuick.Layouts

Rectangle {
    width: 450
    height: 300
    color: "#f8f8f8"

    ColumnLayout {
        Text {
                text: "# Undo Test"
        }
        RowLayout {
                Button {
                        text: "Undo"
                        onClicked: undo.undo()
                        enabled: undo.canUndo
                }
                Text {
                        text: undo.undoName
                }
        }
        RowLayout {
                Button {
                        text: "Redo"
                        onClicked: undo.redo()
                        enabled: undo.canRedo
                }
                Text {
                        text: undo.redoName
                }
        }
        Button {
                text: "Do something"
                onClicked: plugin.invoke("incrementState")
        }
        Text {
                text: plugin.props["counter"]
        }
    }
}
