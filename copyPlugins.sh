#!/bin/bash

VERSION="$1"

pushd ~/Library/Audio/Plug-Ins/Components/
<<<<<<< HEAD
zip -r ~/Desktop/SpatGRIS$VERSION.zip ./SpatGRIS3.component 
popd

pushd ~/Library/Audio/Plug-Ins/VST/
zip -r ~/Desktop/SpatGRIS$VERSION.zip ./SpatGRIS3.vst 
popd

pushd ~/Library/Audio/Plug-Ins/VST3/
zip -r ~/Desktop/SpatGRIS$VERSION.zip ./SpatGRIS3.vst3 
=======
zip -r ~/Desktop/Octogris$VERSION.zip ./Octogris3.component 
popd

pushd ~/Library/Audio/Plug-Ins/VST/
zip -r ~/Desktop/Octogris$VERSION.zip ./Octogris3.vst 
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
popd

#zip -rj ~/Desktop/ZirkOSC.zip ~/Library/Audio/Plug-Ins/Components/ZirkOSC3.component ~/Library/Audio/Plug-Ins/VST/ZirkOSC3.vst 

<<<<<<< HEAD
echo "Created zip file SpatGRIS$VERSION.zip"
=======
echo "Created zip file Octogris$VERSION.zip"
>>>>>>> 2588dc2f3221b0a2cc68818c05101612d949a534
