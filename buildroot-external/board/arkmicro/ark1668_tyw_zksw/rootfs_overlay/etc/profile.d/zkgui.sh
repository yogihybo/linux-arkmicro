#!/bin/sh

export LD_LIBRARY_PATH=/usr/lib:/lib
gocsdk >/dev/null 2>&1 & 
zkgui&
