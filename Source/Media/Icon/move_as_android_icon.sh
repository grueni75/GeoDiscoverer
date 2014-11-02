#!/bin/bash
ICON=$1
ANDROID_DIR=../../Platform/Target/Android/res
mv 120dpi/$1 $ANDROID_DIR/drawable-ldpi/
mv 160dpi/$1 $ANDROID_DIR/drawable-mdpi/
mv 240dpi/$1 $ANDROID_DIR/drawable-hdpi/
mv 320dpi/$1 $ANDROID_DIR/drawable-xhdpi/
mv 480dpi/$1 $ANDROID_DIR/drawable-xxhdpi/
mv 640dpi/$1 $ANDROID_DIR/drawable-xxxhdpi/

