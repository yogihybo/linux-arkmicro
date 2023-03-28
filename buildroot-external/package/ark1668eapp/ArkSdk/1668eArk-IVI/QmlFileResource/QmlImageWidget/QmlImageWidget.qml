import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    id:root
    visible: false
    property int originalWidth: 0
    property int originalHeight: 0
    property real m_Scale : 1.0
    property real scale_step:0.5
    Image{
         id:pixmap
         visible: false
         cache:false
         fillMode: Image.Pad
         anchors.centerIn: parent
         property int index: -1
         property int pixmapRotation: 0
         property int lastPixmapRotation: 0
         width:  sourceSize.width  > root.width ? root.width:sourceSize.width
         height: sourceSize.height > root.height ? root.height:sourceSize.height
         objectName:"pixmapObject"
         onIndexChanged: {
            pixmap.width  = sourceSize.width  > root.width ? root.width:sourceSize.width
            pixmap.height = sourceSize.height > root.height ? root.height:sourceSize.height
         }
         NumberAnimation {
            running: pixmap.pixmapRotation != pixmap.lastPixmapRotation?true:false
            loops: 1
            target: pixmap
            from: pixmap.lastPixmapRotation
            to: pixmap.pixmapRotation
            property: "rotation"
            duration: 1000
        }
         onPixmapRotationChanged: {
            pixmap.lastPixmapRotation = pixmap.pixmapRotation
         }

    }

    AnimatedImage {
        id: animated
        anchors.centerIn: parent
        cache:false
        visible:false
        property int index: -1
        property int animatedRotation: 0
        property int lastAnimatedRotation: 0
        width:  sourceSize.width  > root.width ? root.width:sourceSize.width
        height: sourceSize.height > root.height ? root.height:sourceSize.height
        objectName:"animatedObject"
        onIndexChanged: {
           animated.width  = sourceSize.width  > root.width ? root.width:sourceSize.width
           animated.height = sourceSize.height > root.height ? root.height:sourceSize.height
        }
        NumberAnimation {
           running: animated.animatedRotation != animated.lastAnimatedRotation?true:false
           loops: 1
           target: animated
           from: animated.lastAnimatedRotation
           to:   animated.animatedRotation
           property: "rotation"
           duration: 1000
       }
        onAnimatedRotationChanged: {
           animated.lastAnimatedRotation = animated.animatedRotation
        }
    }
    onWidthChanged: {
        if(pixmap.visible === true)
        {
            if(root.width === 1920)
            {
                root.originalWidth = pixmap.width
                root.originalHeight = pixmap.height
                while(1)
                {
                    pixmap.width = root.originalWidth*(root.scale + root.scale_step)
                    pixmap.height = root.originalHeight*(root.scale + root.scale_step)
                    root.scale = root.scale + root.scale_step
                    if(pixmap.width >=1920 && pixmap.height >= 720)
                    {
                        break;
                    }
                }
            }
            else
            {
                root.scale = 1.0
                pixmap.width = root.originalWidth
                pixmap.height = root.originalHeight
            }
        }

        if(animated.visible === true)
        {
            if(root.width === 1920)
            {
                root.originalWidth = animated.width
                root.originalHeight = animated.height
                while(1)
                {
                    animated.width = root.originalWidth*(root.scale + root.scale_step)
                    animated.height = root.originalHeight*(root.scale + root.scale_step)
                    root.scale = root.scale + root.scale_step
                    if(animated.width >=1920 && animated.height >= 720)
                    {
                        break;
                    }
                }
            }
            else
            {
                root.scale = 1.0
                animated.width = root.originalWidth
                animated.height = root.originalHeight
            }
        }
    }

}
