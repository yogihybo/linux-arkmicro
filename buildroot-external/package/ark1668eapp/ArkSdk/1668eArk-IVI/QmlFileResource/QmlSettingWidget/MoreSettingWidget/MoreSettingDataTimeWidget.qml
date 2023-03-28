import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    width: 800
    height: 420
    visible: true
    property int moveYearIndex : 0
    property int moveYearOffset : 0
    property string yearStr:""

    property int moveMonthIndex : 0
    property int moveMonthOffset : 0
    property string monthStr:""

    property int moveDayIndex : 0
    property int moveDayOffset : 0
    property string dayStr:""

    property int moveHourIndex : 0
    property int moveHourOffset : 0
    property string hourStr:""

    property int moveMinIndex : 0
    property int moveMinOffset : 0
    property string minStr:""
    signal confirmBtnClicked()
    signal cancelBtnClicked()
    Rectangle{
        anchors.fill:parent
        color:"#0E0E0E"
        radius: 20
    }

    ListView{
        id:yearListView
        anchors.left: parent.left
        anchors.leftMargin: 147
        anchors.top:parent.top
        anchors.topMargin: 84
        width: 65
        height: 174
        objectName: "yearListViewObject"
        model:["","1978","1979","1980","1981","1982","1983","1984","1985","1986","1987",
               "1988","1989","1990","1991","1992","1993","1994","1995","1996","1997",
               "1998","1999","2000","2001","2002","2003","2004","2005","2006","2007",
               "2008","2009","2010","2011","2012","2013","2014","2015","2016","2017",
               "2018","2019","2020","2021","2022","2023","2024","2025","2026","2027",
               "2028","2029","2030","2031","2032","2033","2034","2035","2036","2037",
               "2038","2039","2040","2041","2042","2043","2044","2045","2046","2047",
               "2048","2049","2050","2051","2052","2053","2054","2055","2056","2057",""]
        delegate: Rectangle{
            width: 65
            height: 58
            color: "transparent"
            property alias yearOpacity: yearText.opacity
            property alias year_Text: yearText.text
            Text{
                id:yearText
                anchors.fill:parent
                color:"#FFFFFF"
                opacity: (index == (yearListView.currentIndex+1))?1:0.4
                font.pixelSize: 28
                font.family: "Alibaba PuHuiTi"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                text:modelData
            }
        }
        onContentYChanged: {
            if(yearListView.contentY <= 0)
            {
                yearListView.contentY = 0
            }
            if(yearListView.contentY >= 4582)
            {
                yearListView.contentY = 4582;
            }
            root.moveYearIndex   =  (yearListView.contentY/58);
            yearListView.itemAtIndex(root.moveYearIndex).yearOpacity = 0.4;
            yearListView.itemAtIndex(root.moveYearIndex+1).yearOpacity = 1;
            yearListView.itemAtIndex(root.moveYearIndex+2).yearOpacity = 0.4;
        }
        onMovementEnded: {
            root.moveYearIndex   =  (yearListView.contentY/58);
            root.moveYearOffset  =  (yearListView.contentY%58);
            if(root.moveYearOffset >= 26)
            {
                root.moveYearIndex = root.moveYearIndex + 1;
                yearListView.contentY = (58*root.moveYearIndex);
            }
            else if(root.moveYearOffset < 26 && root.moveYearOffset != 0){
                yearListView.contentY = (58*root.moveYearIndex);
            }
            yearListView.currentIndex = root.moveYearIndex;
        }
    }

    Text{
        anchors.left: yearListView.right
        anchors.leftMargin: 1
        anchors.top:parent.top
        anchors.topMargin: 152
        width:30
        height:38
        color: "#FFFFFF"
        font.pixelSize: 28
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text:qsTr("年")
    }

    ListView{
        id:monthListView
        anchors.left: parent.left
        anchors.leftMargin: 277
        anchors.top:parent.top
        anchors.topMargin: 84
        width: 33
        height: 174
        objectName: "monthListViewObject"
        model:["","01","02","03","04","05","06","07","08","09","10","11","12",""]
        delegate: Rectangle{
            width: 33
            height: 58
            color: "transparent"
            property alias monthOpacity: monthText.opacity
            property alias month_Text: monthText.text
            Text{
                id:monthText
                anchors.fill:parent
                color:"#FFFFFF"
                opacity: (index == (monthListView.currentIndex+1))?1:0.4
                font.pixelSize: 28
                font.family: "Alibaba PuHuiTi"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                text:modelData
            }
        }
        onContentYChanged: {
            if(monthListView.contentY <= 0)
            {
                monthListView.contentY = 0
            }
            if(monthListView.contentY >= 638)
            {
                monthListView.contentY = 638;
            }
            root.moveMonthIndex  =  (monthListView.contentY/58);
            monthListView.itemAtIndex(root.moveMonthIndex).monthOpacity = 0.4;
            monthListView.itemAtIndex(root.moveMonthIndex+1).monthOpacity = 1;
            monthListView.itemAtIndex(root.moveMonthIndex+2).monthOpacity = 0.4;
        }
        onMovementEnded: {
            root.moveMonthIndex  =  (monthListView.contentY/58);
            root.moveMonthOffset =  (monthListView.contentY%58);
            if(root.moveMonthOffset >= 26)
            {
                root.moveMonthIndex = root.moveMonthIndex + 1;
                monthListView.contentY = (58*root.moveMonthIndex);
            }
            else if(root.moveMonthOffset < 26 && root.moveMonthOffset != 0){
                monthListView.contentY = (58*root.moveMonthIndex);
            }
            monthListView.currentIndex = root.moveMonthIndex;
        }
    }
    Text{
        anchors.left: monthListView.right
        anchors.leftMargin: 1
        anchors.top:parent.top
        anchors.topMargin: 152
        width:30
        height:38
        color: "#FFFFFF"
        font.pixelSize: 28
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text:qsTr("月")
    }

    ListView{
        id:dayListView
        anchors.left: parent.left
        anchors.leftMargin: 367
        anchors.top:parent.top
        anchors.topMargin: 84
        width: 33
        height: 174
        objectName: "dayListViewObject"
        model:["","01","02","03","04","05","06","07","08","09","10","11","12","13","14","15",
               "16","17","18","19","20","21","22","23","24","25","26","27","28","29","30","31",""]
        delegate: Rectangle{
            width: 33
            height: 58
            color: "transparent"
            property alias dayOpacity: dayText.opacity
            property alias day_Text: dayText.text
            Text{
                id:dayText
                anchors.fill:parent
                color:"#FFFFFF"
                opacity: (index == (dayListView.currentIndex+1))?1:0.4
                font.pixelSize: 28
                font.family: "Alibaba PuHuiTi"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                text:modelData
            }
        }
        onContentYChanged: {
            if(dayListView.contentY <= 0)
            {
                dayListView.contentY = 0
            }
            if(dayListView.contentY >= 1740)
            {
                dayListView.contentY = 1740;
            }
            root.moveDayIndex   =  (dayListView.contentY/58);
            dayListView.itemAtIndex(root.moveDayIndex).dayOpacity = 0.4;
            dayListView.itemAtIndex(root.moveDayIndex+1).dayOpacity = 1;
            dayListView.itemAtIndex(root.moveDayIndex+2).dayOpacity = 0.4;
        }
        onMovementEnded: {
            root.moveDayIndex   =  (dayListView.contentY/58);
            root.moveDayOffset =  (dayListView.contentY%58);
            if(root.moveDayOffset >= 26)
            {
                root.moveDayIndex = root.moveDayIndex + 1;
                dayListView.contentY = (58*root.moveDayIndex);
            }
            else if(root.moveDayOffset < 26 && root.moveDayOffset != 0){
                dayListView.contentY = (58*root.moveDayIndex);
            }
            dayListView.currentIndex = root.moveDayIndex;
        }
    }
    Text{
        anchors.left: dayListView.right
        anchors.leftMargin: 1
        anchors.top:parent.top
        anchors.topMargin: 152
        width:30
        height:38
        color: "#FFFFFF"
        font.pixelSize: 28
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text:qsTr("日")
    }

    ListView{
        id:hourListView
        anchors.left: parent.left
        anchors.leftMargin: 507
        anchors.top:parent.top
        anchors.topMargin: 84
        width: 33
        height: 174
        objectName: "hourListViewObject"
        model:["","01","02","03","04","05","06","07","08","09","10","11","12","13","14","15",
               "16","17","18","19","20","21","22","23",""]
        delegate: Rectangle{
            width: 33
            height: 58
            color: "transparent"
            property alias hourOpacity:hourText.opacity
            property alias hour_Text: hourText.text
            Text{
                id:hourText
                anchors.fill:parent
                color:"#FFFFFF"
                opacity: (index == (hourListView.currentIndex+1))?1:0.4
                font.pixelSize: 28
                font.family: "Alibaba PuHuiTi"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                text:modelData
            }
        }
        onContentYChanged: {
            if(hourListView.contentY < 0)
            {
                hourListView.contentY = 0
            }
            if(hourListView.contentY >= 1276)
            {
                hourListView.contentY = 1276;
            }
            root.moveHourIndex   =  (hourListView.contentY/58);
            hourListView.itemAtIndex(root.moveHourIndex).hourOpacity = 0.4;
            hourListView.itemAtIndex(root.moveHourIndex+1).hourOpacity = 1;
            hourListView.itemAtIndex(root.moveHourIndex+2).hourOpacity = 0.4;
        }
        onMovementEnded: {
            root.moveHourIndex =  (hourListView.contentY/58);
            root.moveHourOffset =  (hourListView.contentY%58);
            if(root.moveHourOffset >= 26)
            {
                root.moveHourIndex = root.moveHourIndex + 1;
                hourListView.contentY = (58*root.moveHourIndex);
            }
            else if(root.moveHourOffset < 26 && root.moveHourOffset != 0){
                hourListView.contentY = (58*root.moveHourIndex);
            }
            hourListView.currentIndex = root.moveHourIndex;
        }
    }

    Text{
        anchors.left: hourListView.right
        anchors.leftMargin: 1
        anchors.top:parent.top
        anchors.topMargin: 152
        width:30
        height:38
        color: "#FFFFFF"
        font.pixelSize: 28
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text:qsTr("时")
    }

    ListView{
        id:minListView
        anchors.left: parent.left
        anchors.leftMargin: 597
        anchors.top:parent.top
        anchors.topMargin: 84
        width: 33
        height: 174
        objectName: "minListViewObject"
        model:["","01","02","03","04","05","06","07","08","09","10","11","12","13","14","15",
               "16","17","18","19","20","21","22","23","24","25","26","27","28","29","30",
               "31","32","33","34","35","36","37","38","39","40","41","42","43","44","45",
               "46","47","48","49","50","51","52","53","54","55","56","57","58","59",""]
        delegate: Rectangle{
            width: 33
            height: 58
            color: "transparent"
            property alias minOpacity: minText.opacity
            property alias min_Text: minText.text
            Text{
                id:minText
                anchors.fill:parent
                color:"#FFFFFF"
                opacity: (index == (hourListView.currentIndex+1))?1:0.4
                font.pixelSize: 28
                font.family: "Alibaba PuHuiTi"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                text:modelData
            }
        }
        onContentYChanged: {
            if(minListView.contentY <= 0)
            {
                minListView.contentY = 0
            }
            if(minListView.contentY >= 3364)
            {
                minListView.contentY = 3364;
            }
            root.moveMinIndex   =  (minListView.contentY/58);
            minListView.itemAtIndex(root.moveMinIndex).minOpacity = 0.4;
            minListView.itemAtIndex(root.moveMinIndex+1).minOpacity = 1;
            minListView.itemAtIndex(root.moveMinIndex+2).minOpacity = 0.4;
        }
        onMovementEnded: {
            root.moveMinIndex  =  (minListView.contentY/58);
            root.moveMinOffset =  (minListView.contentY%58);
            if(root.moveMinOffset >= 26)
            {
                root.moveMinIndex = root.moveMinIndex + 1;
                minListView.contentY = (58*root.moveMinIndex);
            }
            else if(root.moveMinOffset < 26 && root.moveMinOffset != 0){
                minListView.contentY = (58*root.moveMinIndex);
            }
            minListView.currentIndex = root.moveMinIndex;
        }
    }
    Text{
        anchors.left: minListView.right
        anchors.leftMargin: 1
        anchors.top:parent.top
        anchors.topMargin: 152
        width:30
        height:38
        color: "#FFFFFF"
        font.pixelSize: 28
        font.family: "Alibaba PuHuiTi"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text:qsTr("分")
    }

    Rectangle{
        anchors.left: parent.left
        anchors.bottom:yearListView.top
        width: 800
        height: 84
        color:"#0E0E0E"
    }
    Rectangle{
        anchors.left: parent.left
        anchors.top:yearListView.bottom
        width: 800
        height: 84
        color:"#0E0E0E"
    }

    Rectangle{
        anchors.left: parent.left
        anchors.top:parent.top
        anchors.topMargin: 320
        width: 800
        height: 2
        color: "#FFFFFF"
    }
    Button{
        id:cancleBtn
        anchors.left: parent.left
        anchors.leftMargin: 210
        anchors.top:parent.top
        anchors.topMargin: 340
        width: 150
        height: 60
        objectName: "cancleBtnObject"
        background: Rectangle{
            id:cancleBtnBg
            anchors.fill: parent
            color: "#FFFFFF"
            opacity: 0.2
            radius: 50
        }

        Text{
            anchors.fill:parent
            color: "#FFFFFF"
            font.pixelSize: 20
            font.family: "Montserrat"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("取消")
        }
        onPressed:  cancleBtnBg.opacity = 0.4
        onReleased: cancleBtnBg.opacity = 0.2
        onClicked: {
            root.visible = false;
            root.cancelBtnClicked();
        }
    }

    Button{
        id:confirmBtn
        anchors.left: parent.left
        anchors.leftMargin: 440
        anchors.top:parent.top
        anchors.topMargin: 340
        width: 150
        height: 60
        objectName: "confirmBtnObject"
        background: Rectangle{
            id:confirmBtnBg
            anchors.fill: parent
            color: "#0DA8FF"
            radius: 50
        }

        Text{
            anchors.fill:parent
            color: "#FFFFFF"
            font.pixelSize: 20
            font.family: "Montserrat"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text:qsTr("确认")
        }
        onPressed:  confirmBtn.opacity = 0.4
        onReleased: confirmBtn.opacity = 1
        onClicked: {
            root.yearStr = yearListView.itemAtIndex(yearListView.currentIndex+1).year_Text;
            root.monthStr= monthListView.itemAtIndex(monthListView.currentIndex+1).month_Text;
            root.dayStr  = dayListView.itemAtIndex(dayListView.currentIndex+1).day_Text;
            root.hourStr = hourListView.itemAtIndex(hourListView.currentIndex+1).hour_Text;
            root.minStr  = minListView.itemAtIndex(minListView.currentIndex+1).min_Text;
            root.visible = false;
            root.confirmBtnClicked();
        }
    }
}
