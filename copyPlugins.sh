#!/bin/bash

VERSION="$1"

pushd ~/Library/Audio/Plug-Ins/Components/
zip -r ~/Desktop/SpatGRIS$VERSION.zip ./SpatGRIS3.component 
popd

pushd ~/Library/Audio/Plug-Ins/VST/
zip -r ~/Desktop/SpatGRIS$VERSION.zip ./SpatGRIS3.vst 
popd

pushd ~/Library/Audio/Plug-Ins/VST3/
zip -r ~/Desktop/SpatGRIS$VERSION.zip ./SpatGRIS3.vst3 
popd

#zip -rj ~/Desktop/ZirkOSC.zip ~/Library/Audio/Plug-Ins/Components/ZirkOSC3.component ~/Library/Audio/Plug-Ins/VST/ZirkOSC3.vst 

echo "Created zip file SpatGRIS$VERSION.zip"
