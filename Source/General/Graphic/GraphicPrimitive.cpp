//============================================================================
// Name        : GraphicPrimitive.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// GeoDiscoverer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GeoDiscoverer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GeoDiscoverer.  If not, see <http://www.gnu.org/licenses/>.
//
//============================================================================


#include <Core.h>

namespace GEODISCOVERER {

// Constructor
GraphicPrimitive::GraphicPrimitive() {
  type=GraphicTypePrimitive;
  setTexture(core->getScreen()->getTextureNotDefined());
  setX(0);
  setY(0);
  setZ(0);
  setAngle(0);
  fadeDuration=core->getConfigStore()->getIntValue("Graphic","fadeDuration");
  fadeEndTime=0;
  fadeStartTime=0;
  blinkDuration=core->getConfigStore()->getIntValue("Graphic","blinkDuration");
  blinkPeriod=core->getConfigStore()->getIntValue("Graphic","blinkPeriod");
  blinkMode=GraphicBlinkIdle;
  destroyTexture=true;
  animator=NULL;
}

// Destructor
GraphicPrimitive::~GraphicPrimitive() {
  deinit();
}

// Deinits the primitive
void GraphicPrimitive::deinit() {
  if ((texture!=core->getScreen()->getTextureNotDefined())&&(destroyTexture)) {
    core->getScreen()->destroyTextureInfo(texture);
    texture=core->getScreen()->getTextureNotDefined();
  }
}

// Sets a new fade target
void GraphicPrimitive::setFadeAnimation(TimestampInMicroseconds startTime, GraphicColor startColor, GraphicColor endColor) {
  TimestampInMicroseconds redDiff=abs(endColor.getRed()-startColor.getRed());
  TimestampInMicroseconds greenDiff=abs(endColor.getGreen()-startColor.getGreen());
  TimestampInMicroseconds blueDiff=abs(endColor.getBlue()-startColor.getBlue());
  TimestampInMicroseconds alphaDiff=abs(endColor.getAlpha()-startColor.getAlpha());
  TimestampInMicroseconds duration;
  fadeStartTime=startTime;
  if (redDiff>greenDiff)
    duration=redDiff;
  else
    duration=greenDiff;
  if (blueDiff>duration)
    duration=blueDiff;
  if (alphaDiff>duration)
    duration=alphaDiff;
  duration=fadeDuration*duration/std::numeric_limits<UByte>::max();
  fadeEndTime=startTime+duration;
  fadeStartColor=startColor;
  fadeEndColor=endColor;
}

// Activates or disactivates blinking
void GraphicPrimitive::setBlinkAnimation(bool active, GraphicColor highlightColor) {
  if (active) {
    blinkHighlightColor=highlightColor;
    blinkMode=GraphicBlinkFadeToHighlightColor;
  } else {
    blinkMode=GraphicBlinkIdle;
  }
  fadeStartTime=fadeEndTime;
}

// Lets the primitive work (e.g., animation)
bool GraphicPrimitive::work(TimestampInMicroseconds currentTime) {
  bool changed=false;

  // Blink animation required?
  if (fadeStartTime==fadeEndTime) {
    switch(blinkMode) {
      case GraphicBlinkFadeToHighlightColor:
        blinkOriginalColor=this->color;
        setFadeAnimation(currentTime+blinkPeriod,blinkOriginalColor,blinkHighlightColor);
        blinkMode=GraphicBlinkFadeToOriginalColor;
        //DEBUG("primitive=0x%08x blinkMode=fadeToHighlightColor t=%llu fadeStartTime=%llu fadeEndTime=%llu",this,currentTime,fadeStartTime,fadeEndTime);
        break;
      case GraphicBlinkFadeToOriginalColor:
        setFadeAnimation(currentTime+blinkDuration,blinkHighlightColor,blinkOriginalColor);
        blinkMode=GraphicBlinkFadeToHighlightColor;
        //DEBUG("primitive=0x%08x blinkMode=fadeToOriginalColor t=%llu fadeStartTime=%llu fadeEndTime=%llu",this,currentTime,fadeStartTime,fadeEndTime);
        break;
    }
  }

  // Fade animation required?
  if ((fadeStartTime<=currentTime)&&(fadeStartTime!=fadeEndTime)) {
    changed=true;
    if (currentTime<=fadeEndTime) {
      Int elapsedTime=currentTime-fadeStartTime;
      Int duration=fadeEndTime-fadeStartTime;
      Int redDiff=(Int)fadeEndColor.getRed()-(Int)fadeStartColor.getRed();
      Int red=redDiff*elapsedTime/duration+fadeStartColor.getRed();
      color.setRed((UByte)red);
      Int greenDiff=(Int)fadeEndColor.getGreen()-(Int)fadeStartColor.getGreen();
      Int green=greenDiff*elapsedTime/duration+fadeStartColor.getGreen();
      color.setGreen((UByte)green);
      Int blueDiff=(Int)fadeEndColor.getBlue()-(Int)fadeStartColor.getBlue();
      Int blue=blueDiff*elapsedTime/duration+fadeStartColor.getBlue();
      color.setBlue((UByte)blue);
      Int alphaDiff=(Int)fadeEndColor.getAlpha()-(Int)fadeStartColor.getAlpha();
      Int alpha=alphaDiff*elapsedTime/duration+fadeStartColor.getAlpha();
      color.setAlpha((UByte)alpha);
    } else {
      color=fadeEndColor;
      fadeStartTime=fadeEndTime;
    }
  } else {
    /*if ((blinkMode==GraphicBlinkFadeToHighlightColor)) {
      DEBUG("primitive=0x%08x blinkMode=fadeToHighlightColor t=%llu fadeStartTime=%llu fadeEndTime=%llu",this,currentTime,fadeStartTime,fadeEndTime);
    }
    if ((blinkMode==GraphicBlinkFadeToOriginalColor)) {
      DEBUG("primitive=0x%08x blinkMode=fadeToOriginalColor t=%llu fadeStartTime=%llu fadeEndTime=%llu",this,currentTime,fadeStartTime,fadeEndTime);
    }*/
  }

  // If the primitive has changed, redraw it
  if (isUpdated) {
    changed=true;
    isUpdated=false;
  }

  return changed;
}

// Recreates any textures or buffers
void GraphicPrimitive::invalidate() {
}

// Recreates any textures or buffers
void GraphicPrimitive::optimize() {
}

}
