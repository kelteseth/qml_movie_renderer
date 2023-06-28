import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    visible: true
    width: 1366
    height: 620
    title: "QML Movie Renderer"

    MovieRenderer {
        id: movieRenderer
    }
    SplitView {
        id: wrapper
        anchors.fill: parent
        anchors.margins: 40

        ColumnLayout {
            SplitView.preferredWidth: wrapper.width * .5
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    text: "QML File"
                }
                TextField {
                    id: qmlFileTextField
                    Layout.fillWidth: true
                    text: "file:///C:/Users/Ryzen-7950X/Documents/hello.qml"
                }
                Button {
                    text: "Select qml file"
                    onClicked: fileDialog.open()
                }
                FileDialog {
                    id: fileDialog
                    title: "Please choose a file"
                    nameFilters: ["qml files (*.qml)"]
                    onAccepted: {
                        qmlFileTextField.text = fileDialog.selectedFile;
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    text: "Output Size"
                }
                TextField {
                    id: widthSpinBox
                    Layout.fillWidth: true
                    text: "1280"
                }
                Label {
                    text: "X"
                }
                TextField {
                    id: heightSpinBox
                    Layout.fillWidth: true
                    text: "720"
                }
            }

            RowLayout {
                Label {
                    text: "Duration (ms)"
                }
                TextField {
                    id: durationSpinBox
                    Layout.fillWidth: true
                    text: "3000"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: "Frames per Second"
                }
                TextField {
                    id: fpsSpinBox
                    Layout.fillWidth: true
                    text: "10"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: "Output Directory"
                }
                TextField {
                    id: outputDirectoryTextField
                    text: "file:///C:/workspace/Qt/build_QmlOffscreenRenderer_64bit_Debug"
                    Layout.fillWidth: true
                }
                Button {
                    text: "..."
                    onClicked: folderDialog.open()

                    // implement directory dialog functionality here
                    FolderDialog {
                        id: folderDialog
                        onAccepted: {
                            outputDirectoryTextField.text = folderDialog.selectedFolder;
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: "Output Filename"
                }
                TextField {
                    id: outputFilenameTextField
                    Layout.fillWidth: true
                    text: "frame"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: "Image Format"
                }
                ComboBox {
                    id: imageFormatComboBox
                    Layout.fillWidth: true
                    textRole: "text"
                    valueRole: "value"
                    model: [{
                            "value": "png",
                            "text": "png"
                        }]
                }
            }

            Button {
                text: "Render Movie"
                onClicked: {
                    movieRenderer.renderMovie(qmlFileTextField.text, outputFilenameTextField.text, outputDirectoryTextField.text, imageFormatComboBox.currentValue, Qt.size(widthSpinBox.text, heightSpinBox.text), 1, durationSpinBox.text, fpsSpinBox.text);
                }
            }

            ProgressBar {
                Layout.fillWidth: true
                from: 0
                value: movieRenderer.progress
                to: 100
            }
        }

        Loader {
            SplitView.preferredWidth: wrapper.width * .5
            source: Qt.resolvedUrl(qmlFileTextField.text)
        }
    }
}
