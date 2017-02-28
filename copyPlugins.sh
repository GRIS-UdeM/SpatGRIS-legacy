#!/bin/bash

VERSION="$1"
MESSAGE="$2"

pushd ~/Library/Audio/Plug-Ins/Components/
zip -r ~/Desktop/SpatGRIS$VERSION.zip ./SpatGRIS.component 
popd

pushd ~/Library/Audio/Plug-Ins/VST/
zip -r ~/Desktop/SpatGRIS$VERSION.zip ./SpatGRIS.vst 
popd

#pushd ~/Library/Audio/Plug-Ins/VST3/
#zip -r ~/Desktop/SpatGRIS$VERSION.zip ./SpatGRIS.vst3 
#popd

#zip -rj ~/Desktop/ZirkOSC.zip ~/Library/Audio/Plug-Ins/Components/ZirkOSC3.component ~/Library/Audio/Plug-Ins/VST/ZirkOSC3.vst 

echo "Created zip file SpatGRIS$VERSION.zip"
git checkout master

git tag -a "v$VERSION" -m "$MESSAGE"
git push origin "v$VERSION"
git push

echo "Done tag v$VERSION and message $MESSAGE"
