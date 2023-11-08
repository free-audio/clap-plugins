import QtQuick 2.1
import QtQuick.Controls 2.1

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

        return text;
    }

    function updateText() {
        display.text = computeText();
    }

    Component.onCompleted: {
        trackInfo.updated.connect(updateText);
    }
}
