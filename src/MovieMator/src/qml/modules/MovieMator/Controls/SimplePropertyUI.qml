/*
 * Copyright (c) 2011-2016 Meltytech, LLC
 *
 * Copyright (c) 2016-2019 EffectMatrix Inc.
 *
 */
import QtQuick 2.1
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.1

Column {
    id: root
    Layout.columnSpan: 4
    Layout.fillWidth: true
    spacing: 4

    property var properties: []

    Label {
        text: qsTr("Custom Properties")
        font.bold: true
    }

    Repeater {
        model: root.properties

        RowLayout {
            width: parent.width
            height: propField.height
            Label {
                Layout.preferredWidth: editButton.width
                Layout.minimumWidth: implicitWidth
                anchors {
                    verticalCenter: propField.verticalCenter
                }
                text: modelData
            }
            TextField {
                id: propField
                Layout.fillWidth: true
                text: filter.get(modelData)
                onTextChanged: filter.set(modelData, text)
            }
        }
    }
}
