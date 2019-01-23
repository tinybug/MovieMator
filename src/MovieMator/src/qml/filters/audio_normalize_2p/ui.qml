
import QtQuick 2.1
import QtQuick.Dialogs 1.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import MovieMator.Controls 1.0

Item {
    width: 300
    height: 250
    
    function setStatus( inProgress ) {
        if (inProgress) {
            status.text = qsTr('Analyzing...')
        }
        else if (filter.get("results").length > 0 ) {
            status.text = qsTr('Analysis complete.')
        }
        else
        {
            status.text = qsTr('Click "Analyze" to use this filter.')
        }
    }

    Connections {
        target: filter
        onAnalyzeFinished: {
            setStatus(false)
            button.enabled = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label {
                text: qsTr('Target Loudness')
                color: '#ffffff'
            }
            SliderSpinner {
                id: programSlider
                minimumValue: -50.0
                maximumValue: -10.0
                decimals: 1
                suffix: ' LUFS'
                spinnerWidth: 100
                value: filter.getDouble('program')
                onValueChanged: filter.set('program', value)
            }
            UndoButton {
                onClicked: programSlider.value = -23.0
            }
        }

        RowLayout {
            Button {
                id: button
                text: qsTr('Analyze')

                onClicked: {
                    button.enabled = false
                    filter.analyze(true);
                }
            }
            Label {
                id: status
                Component.onCompleted: {
                    setStatus(false)
                }
            }
        }

        Item {
            Layout.fillHeight: true;
        }
    }
}