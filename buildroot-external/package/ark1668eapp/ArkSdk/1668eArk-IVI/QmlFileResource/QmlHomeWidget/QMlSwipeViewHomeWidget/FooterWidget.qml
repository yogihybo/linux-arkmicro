import QtQuick 2.0
Item {
    id:root
    width: 200
    height: 50
    visible: true
    property int currentIndex: 0
    Rectangle{
        anchors.fill:parent
        color:"transparent"
        Row{
            anchors.left:parent.left
            anchors.leftMargin: 64
            anchors.top:parent.top
            anchors.topMargin: 16
            width: 64
            height: 18
            spacing:5
            Repeater{
                id:repeater
                model: 3
                delegate: Rectangle{
                    id:delRect
                    width: 18
                    height: 18
                    border.color: "#FFFFFF"
                    border.width: 2
                    opacity: 0.5
                    color: "transparent"
                    radius: 9
                    property alias delRectColor: delRect.color

                }

            }
        }
    }
    Component.onCompleted: {
        if(root.currentIndex=== 0)
        {
            repeater.itemAt(root.currentIndex).delRectColor = "#FFFFFF";
        }
    }
    onCurrentIndexChanged: {
            for(var i = 0;i<3;i++)
            {
                if(i === root.currentIndex)
                {
                    repeater.itemAt(i).delRectColor = "#FFFFFF";
                }
                else
                {
                    repeater.itemAt(i).delRectColor = "transparent";
                }
            }
    }


}
