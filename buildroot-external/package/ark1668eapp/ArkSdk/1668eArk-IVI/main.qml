import QtQuick 2.9
import QtQuick.Controls 2.0
import QtQuick.Window 2.2
import QtQuick.Layouts 1.2
import "./QmlFileResource"
ApplicationWindow {
    id:root
    visible: true
    width: 1920
    height: 720
    flags: Qt.BypassWindowManagerHint | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    MainWidget{
        id:mainWidget
        x:0
        y:0
        objectName:"mainWidgetObject"
    }
}
