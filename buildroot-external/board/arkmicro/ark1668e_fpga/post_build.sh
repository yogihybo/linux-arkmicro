#!/bin/bash

cd ${TARGET_DIR}/usr/lib
ln -sf libMali.so libOpenVG.so
ln -sf libMali.so libOpenVGU.so
ln -sf libMali.so libGLESv2.so.2.0
ln -sf libMali.so libEGL.so.1.4
ln -sf libGLESv2.so.2.0 libGLESv2.so.2
ln -sf libGLESv2.so.2 libGLESv2.so
ln -sf libEGL.so.1.4 libEGL.so.1
ln -sf libEGL.so.1 libEGL.so
