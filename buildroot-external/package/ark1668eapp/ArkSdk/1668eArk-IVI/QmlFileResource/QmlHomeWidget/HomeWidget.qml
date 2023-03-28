import QtQuick 2.9
import QtQuick.Controls 2.15
import QtQuick.Window 2.2
import "./QMlSwipeViewHomeWidget/"
Item {
    id:root
    width: 1760
    height: 720
    visible: true
    property int swipeViewCurrentIndex: 0
    SwipeView{
        id:swipeView
        anchors.fill:parent
        objectName: "swipeViewObject"
        FirstHomeWidget{
            id:firstHomeWidget
            objectName:"firstHomeWidgetObject"
        }
        Loader{
            id:secondHomeWidget_loader
            visible: true
            asynchronous: true
        }
        Component{
            id:secondHomeWidgetComponent
            SecondHomeWidget{
                id:secondHomeWidget
            }
        }
        Loader{
            id:thirdHomeWidget_loader
            visible: true
            asynchronous: true
        }
        Component{
            id:thirdHomeWidgetComponent
            ThirdHomeWidget{
                id:thirdHomeWidget
            }
        }
        onCurrentIndexChanged:{
            root.swipeViewCurrentIndex  = swipeView.currentIndex
            firstHomeWidget.currentPage = swipeView.currentIndex
        }
        Component.onCompleted: {
            secondHomeWidget_loader.sourceComponent = secondHomeWidgetComponent;
            thirdHomeWidget_loader.sourceComponent = thirdHomeWidgetComponent;
        }
    }
}
