#!/bin/bash
# Script that copies files modified to the folder where I'm compiling the program

cp src/network/quickplay/qUtilities.h ../armagetronad/src/network/quickplay/qUtilities.h
cp src/network/quickplay/qUtilities.cpp ../armagetronad/src/network/quickplay/qUtilities.cpp
cp src/network/quickplay/quickplayMaster.cpp ../armagetronad/src/network/quickplay/quickplayMaster.cpp
cp src/Makefile.am ../armagetronad/src/Makefile.am
cp src/tron/gServerBrowser.cpp ../armagetronad/src/tron/gServerBrowser.cpp
cp src/tron/gServerBrowser.h ../armagetronad/src/tron/gServerBrowser.h
cp src/tron/gGame.h ../armagetronad/src/tron/gGame.h
cp src/tron/gGame.cpp ../armagetronad/src/tron/gGame.cpp
