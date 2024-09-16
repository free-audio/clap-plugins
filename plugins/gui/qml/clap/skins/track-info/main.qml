import QtQuick 6.0
import QtQuick.Controls 6.0

Rectangle {
    width: 450
    height: 490
    color: "#f8f8f8"

    Text {
        id: display
        text: computeText()
        font.pointSize: 20
    }

    function computeText() {
        if (!trackInfo.hasInfo)
            return "No info";

        var text = "";
        text += "Name: " + trackInfo.name + "\n";
        text += "Color: " + trackInfo.color + "\n";
        text += "Flags: " + trackInfo.flags + "\n";
        text += "Audio Port Type: " + trackInfo.audioPortType + "\n";
        text += "Audio Channel Count: " + trackInfo.audioChannelCount + "\n";
        text += "IsForReturn: " + trackInfo.isForReturn + "\n";
        text += "IsForBus: " + trackInfo.isForBus + "\n";
        text += "IsForMaster: " + trackInfo.isForMaster + "\n";

        return text;
    }

    function updateText() {
        display.text = computeText();
    }

    Component.onCompleted: {
        trackInfo.updated.connect(updateText);
    }
}
