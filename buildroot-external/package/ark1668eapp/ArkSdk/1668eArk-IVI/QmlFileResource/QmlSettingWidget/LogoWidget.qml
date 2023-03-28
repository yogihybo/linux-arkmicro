import QtQuick 2.0

Item {
    id:root
    width: 520
    height: 720
    visible: true

    Rectangle{
        anchors.fill:parent
        color: "#000000"
    }
    Rectangle{
        id:logo
        anchors.centerIn: parent
        width: 370
        height: 397
        color:"transparent"
        Image{
            anchors.fill:parent
            source: "qrc:/images/SettingWidget/NISSAN.png"
        }
    }

}
