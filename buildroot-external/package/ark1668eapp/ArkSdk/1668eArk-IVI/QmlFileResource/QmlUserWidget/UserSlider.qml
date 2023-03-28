/*************************************************************************/
/*                          Copyright Notice                             */
/* The code can not be copied or provided to other people without the    */
/* permission of Zhang Hao,otherwise intellectual property infringement  */
/* will be prosecuted.If you have any questions,please send me an email. */
/* My email is kingderzhang@foxmail.com. The final interpretation rights */
/* are interpreted by ZhangHao.                                          */
/*                  Copyright (C) ZhangHao All rights reserved           */
/*************************************************************************/

import QtQuick 2.0
import QtQuick.Controls 2.0
/**
 * @ClassName: UserSlider
 * @Description: 用户自定义滑块
 * @Autor: kinderzhang@foxmail.com
 * @date: 2018-11-23 10:50:38
 * @Version: 1.0.0
 * @update_autor
 * @update_time
**/
ProgressBar{
    property int currentValue: 0 // 外部需要显示的值
    property int fromValue: 0  // 起始值
    property int toValue: 100  // 结束值
    property color backColor: "gray" // 背景条颜色
    property double backOpacity: 0.5 // 背景条透明度
    property color currenColor: "white" // 显示值颜色
    property color circleColor: "white" // 显示值圆圈颜色
    property color textColor: "white" //显示数值的颜色
    property bool clickStatus: false
    signal  sliderReleased(int value)
    onCurrentValueChanged: {
        if(currentValue>=fromValue&&currentValue<=toValue){
            value = currentValue / (toValue - fromValue)
            circle.x = value * (width - circle.width)

        }else{
            if(currentValue>toValue){
                currentValue = toValue
            }else if(currentValue<fromValue){
                currentValue = fromValue
            }
        }
    }

    Component.onCompleted: {
        circle.x = currentValue / (toValue - fromValue) * (width - circle.width)
        backgraycircle.opacity = 0
    }
    MouseArea{
        id: mouseArea
        anchors.centerIn: parent
        width: parent.width
        height: backgraycircle.height
        hoverEnabled: true
        onEntered: {
            if(!clickStatus)
                backgraycircle.opacity = 0.2
            else
                backgraycircle.opacity = 0.5
        }

        onExited: {
            if(!clickStatus)
                backgraycircle.opacity = 0
        }

        onPressed: {
            clickStatus = true
            currentValue = mouse.x / width * (toValue-fromValue)
            selectedAnimation.start()
        }

        onReleased: {
            clickStatus = false
            sliderReleased(currentValue)
            unSelectedAnimation.start()
        }

        onPositionChanged: { //鼠标按下后改变位置
            if(clickStatus){
                if(mouseX > -parent.height * 0.2  && mouseX <parent.width + parent.height)
                    currentValue = mouseX / width * (toValue-fromValue)
            }
        }
    }

    Item {
        id: circle
        anchors.verticalCenter: parent.verticalCenter
        width: parent.height * 3
        height: parent.height * 3
        Rectangle{//灰色背景
            id: backgraycircle
            anchors.centerIn: parent
            width: parent.width * 2
            height: parent.width * 2
            opacity: 0
            radius: height / 2
            color: "#8f8d8d"
        }

        Rectangle{ //位置圆
            id: backWhiteCircle
            anchors.fill: parent
            radius: height / 2
            color: circleColor
        }

        Text{
            id: showtext
            anchors.top: circle.bottom
            anchors.topMargin: 15
            anchors.horizontalCenter: circle.horizontalCenter
            font.pointSize: parent.height
            font.family: "Montserrat"
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            color: textColor
            opacity: 0
            //text: currentValue
            onTextChanged: {
                textAnimation.restart()
            }
            PropertyAnimation{
                id: textAnimation
                target: showtext
                property: "opacity"
                easing.type: Easing.InCubic
                duration: 1000
                from: 1
                to: 0
            }
        }

    }

    ParallelAnimation{ //点击动画
        id: selectedAnimation
        PropertyAnimation{
            target: backgraycircle
            property: "opacity"
            to: 0.5
            //            from: 0
            duration: 300
        }
        PropertyAnimation{
            target: backWhiteCircle
            property: "scale"
            to: 1.3
            //            from: 1
            duration: 300
        }
    }
    ParallelAnimation{ //取消点击动画
        id: unSelectedAnimation
        PropertyAnimation{
            target: backgraycircle
            property: "opacity"
            //            from: 0.5
            to: 0
            duration: 300
        }
        PropertyAnimation{
            target: backWhiteCircle
            property: "scale"
            //            from: 1.3
            to: 1
            duration: 300
        }
    }
}
