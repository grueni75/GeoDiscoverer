//============================================================================
// Name        : GraphicPrimitive.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2016 Matthias Gruenewald
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
GraphicPrimitive::GraphicPrimitive(Screen *screen) {
  type=GraphicTypePrimitive;
  this->screen=screen;
  setTexture(Screen::getTextureNotDefined());
  setX(0);
  setY(0);
  setZ(0);
  setAngle(0);
  setScale(1.0);
  destroyTexture=true;
  animator=NULL;
  fadeEndTime=0;
  fadeStartTime=0;
  fadeInfinite=false;
  rotateStartTime=0;
  rotateEndTime=0;
  rotateInfinite=false;
  rotateAnimationType=GraphicRotateAnimationTypeLinear;
  scaleStartTime=0;
  scaleEndTime=0;
  scaleInfinite=false;
  translateStartTime=0;
  translateEndTime=0;
  translateInfinite=false;
  translateAnimationType=GraphicTranslateAnimationTypeLinear;
  textureStartTime=0;
  textureEndTime=0;
  textureInfinite=false;
  isUpdated=false;
  lifeEnd=0;
}

// Destructor
GraphicPrimitive::~GraphicPrimitive() {
  deinit();
}

// Deinits the primitive
void GraphicPrimitive::deinit() {
  if ((texture!=Screen::getTextureNotDefined())&&(destroyTexture)) {
    std::stringstream source;
    source << "GraphicPrimitive (type=" << type << ")";
    screen->destroyTextureInfo(texture,source.str());
    texture=Screen::getTextureNotDefined();
  }
}

// Sets a new texture target
void GraphicPrimitive::setTextureAnimation(TimestampInMicroseconds startTime, GraphicTextureInfo startTexture, GraphicTextureInfo endTexture, bool infinite, TimestampInMicroseconds duration) {
  textureDuration=duration;
  textureStartTime=startTime;
  textureEndTime=startTime+duration;
  textureStartInfo=startTexture;
  textureEndInfo=endTexture;
  if (duration==0)
    texture=endTexture;
  fadeInfinite=infinite;
}

// Sets a new fade target
void GraphicPrimitive::setFadeAnimation(TimestampInMicroseconds startTime, GraphicColor startColor, GraphicColor endColor, bool infinite, TimestampInMicroseconds duration) {
  fadeDuration=duration;
  fadeStartTime=startTime;
  fadeEndTime=startTime+duration;
  fadeStartColor=startColor;
  fadeEndColor=endColor;
  if (duration==0)
    color=endColor;
  fadeInfinite=infinite;
}

// Sets a new rotation target
void GraphicPrimitive::setRotateAnimation(TimestampInMicroseconds startTime, double startAngle, double endAngle, bool infinite, TimestampInMicroseconds duration, GraphicRotateAnimationType animationType) {
  rotateDuration=duration;
  rotateStartTime=startTime;
  rotateEndTime=startTime+duration;
  rotateStartAngle=startAngle;
  rotateEndAngle=endAngle;
  if (duration==0)
    angle=endAngle;
  rotateInfinite=infinite;
  rotateAnimationType=animationType;
}

// Sets a new scale target
void GraphicPrimitive::setScaleAnimation(TimestampInMicroseconds startTime, double startFactor, double endFactor, bool infinite, TimestampInMicroseconds duration) {
  scaleDuration=duration;
  scaleStartTime=startTime;
  scaleEndTime=startTime+duration;
  scaleStartFactor=startFactor;
  scaleEndFactor=endFactor;
  if (duration==0)
    scale=endFactor;
  scaleInfinite=infinite;
}

// Sets a new translate target
void GraphicPrimitive::setTranslateAnimation(TimestampInMicroseconds startTime, Int startX, Int startY, Int endX, Int endY, bool infinite, TimestampInMicroseconds duration, GraphicTranslateAnimationType animationType) {
  translateDuration=duration;
  translateStartTime=startTime;
  translateEndTime=startTime+duration;
  translateStartX=startX;
  translateStartY=startY;
  translateEndX=endX;
  translateEndY=endY;
  if (duration==0) {
    x=endX;
    y=endY;
  }
  translateInfinite=infinite;
  translateAnimationType=animationType;
}

// Lets the primitive work (e.g., animation)
bool GraphicPrimitive::work(TimestampInMicroseconds currentTime) {
  bool changed=false;

  // Infinite fade animation required?
  if (fadeStartTime==fadeEndTime) {
    if (fadeInfinite) {
      if (color==fadeStartColor)
        setFadeAnimation(currentTime,fadeStartColor,fadeEndColor,true,fadeDuration);
      if (color==fadeEndColor)
        setFadeAnimation(currentTime,fadeEndColor,fadeStartColor,true,fadeDuration);
    }  else {
        // Some more parameters in the list?
        if (fadeAnimationSequence.size()>0) {
          setNextFadeAnimationStep();
        }
    }
  }

  // Fade animation required?
  if ((fadeStartTime<=currentTime)&&(fadeStartTime!=fadeEndTime)) {
    changed=true;
    if (currentTime<=fadeEndTime) {
      Int elapsedTime=currentTime-fadeStartTime;
      Int duration=fadeEndTime-fadeStartTime;
      double factor=(double)elapsedTime/(double)duration;
      Int redDiff=(Int)fadeEndColor.getRed()-(Int)fadeStartColor.getRed();
      Int red=redDiff*factor+fadeStartColor.getRed();
      color.setRed((UByte)red);
      Int greenDiff=(Int)fadeEndColor.getGreen()-(Int)fadeStartColor.getGreen();
      Int green=greenDiff*factor+fadeStartColor.getGreen();
      color.setGreen((UByte)green);
      Int blueDiff=(Int)fadeEndColor.getBlue()-(Int)fadeStartColor.getBlue();
      Int blue=blueDiff*factor+fadeStartColor.getBlue();
      color.setBlue((UByte)blue);
      Int alphaDiff=(Int)fadeEndColor.getAlpha()-(Int)fadeStartColor.getAlpha();
      Int alpha=alphaDiff*factor+fadeStartColor.getAlpha();
      color.setAlpha((UByte)alpha);
    } else {
      color=fadeEndColor;
      fadeStartTime=fadeEndTime;
    }
  }

  // Infinite texture animation required?
  if (textureStartTime==textureEndTime) {
    if (textureInfinite) {
      if (texture==textureEndInfo)
        setTextureAnimation(currentTime,textureEndInfo,textureStartInfo,true,textureDuration);
      if (texture==textureStartInfo)
        setTextureAnimation(currentTime,textureStartInfo,textureEndInfo,true,textureDuration);
    }  else {

      // Some more parameters in the list?
      if (textureAnimationSequence.size()>0) {
        setNextTextureAnimationStep();
      }
    }
  }

  // Texture animation required?
  if ((textureStartTime<=currentTime)&&(textureStartTime!=textureEndTime)) {
    if (currentTime<textureEndTime) {
      if (texture!=textureStartInfo) {
        texture=textureStartInfo;
        changed=true;
      }
    } else {
      texture=textureEndInfo;
      textureStartTime=textureEndTime;
      changed=true;
    }
  }

  // Infinite rotation animation required?
  if (rotateStartTime==rotateEndTime) {
    if (rotateInfinite) {
      setRotateAnimation(currentTime,rotateStartAngle,rotateEndAngle,true,rotateDuration,rotateAnimationType);
    }  else {

      // Some more parameters in the list?
      if (rotateAnimationSequence.size()>0) {
        setNextRotateAnimationStep();
      }
    }
  }

  // Rotate animation required?
  if ((rotateStartTime<=currentTime)&&(rotateStartTime!=rotateEndTime)) {
    changed=true;
    if (currentTime<=rotateEndTime) {
      Int elapsedTime=currentTime-rotateStartTime;
      Int duration=rotateEndTime-rotateStartTime;
      double angleDiff=rotateEndAngle-rotateStartAngle;
      double speed,accel;
      switch(rotateAnimationType) {
        case GraphicRotateAnimationTypeLinear:
          speed=(double)elapsedTime/(double)duration;
          angle=angleDiff*speed+rotateStartAngle;
          break;
        case GraphicRotateAnimationTypeAccelerated:
          accel=angleDiff/(((double)duration)*((double)duration)/4.0);
          speed=accel*((double)duration)/2;
          if (elapsedTime<=duration/2) {
            angle=rotateStartAngle+accel*((double)elapsedTime)*((double)elapsedTime)/2;
          } else {
            double t=((double)elapsedTime)-((double)duration)/2;
            angle=rotateStartAngle+angleDiff/2+speed*t-accel*t*t/2;
          }
          break;
        default:
          FATAL("animation type is not supported",NULL);
      }
    } else {
      angle=rotateEndAngle;
      rotateStartTime=rotateEndTime;
    }
  }

  // Infinite scale animation required?
  if (scaleStartTime==scaleEndTime) {
    if (scaleInfinite) {
      if (scale==scaleEndFactor)
        setScaleAnimation(currentTime,scaleEndFactor,scaleStartFactor,true,scaleDuration);
      if (scale==scaleStartFactor)
        setScaleAnimation(currentTime,scaleStartFactor,scaleEndFactor,true,scaleDuration);
    } else  {

      // Some more parameters in the list?
      if (scaleAnimationSequence.size()>0) {
        setNextScaleAnimationStep();
      }
    }
  }

  // Scale animation required?
  if ((scaleStartTime<=currentTime)&&(scaleStartTime!=scaleEndTime)) {
    changed=true;
    if (currentTime<=scaleEndTime) {
      Int elapsedTime=currentTime-scaleStartTime;
      double factor=(double)elapsedTime/(double)scaleDuration;
      double scaleDiff=scaleEndFactor-scaleStartFactor;
      scale=scaleDiff*factor+scaleStartFactor;
    } else {
      scale=scaleEndFactor;
      scaleStartTime=scaleEndTime;
    }
  }

  // Infinite translate animation required?
  if (translateStartTime==translateEndTime) {
    if (translateInfinite) {
      if ((x==translateEndX)&&(y==translateEndY))
        setTranslateAnimation(currentTime,translateEndX,translateEndY,translateStartX,translateStartY,true,translateDuration,GraphicTranslateAnimationTypeLinear);
      if ((x==translateStartX)&&(y==translateStartY))
        setTranslateAnimation(currentTime,translateStartX,translateStartY,translateEndX,translateEndY,true,translateDuration,GraphicTranslateAnimationTypeLinear);
    } else  {

      // Some more parameters in the list?
      if (translateAnimationSequence.size()>0) {
        setNextTranslateAnimationStep();
      }
    }
  }

  // Translate animation required?
  if ((translateStartTime<=currentTime)&&(translateStartTime!=translateEndTime)) {
    changed=true;
    if (currentTime<=translateEndTime) {
      Int elapsedTime=currentTime-translateStartTime;
      Int duration=translateEndTime-translateStartTime;
      Int translateDiffX=translateEndX-translateStartX;
      Int translateDiffY=translateEndY-translateStartY;
      double speedX,speedY,accelX,accelY;
      switch(translateAnimationType) {
        case GraphicTranslateAnimationTypeLinear:
          speedX=(double)elapsedTime/(double)duration;
          speedY=speedX;
          x=(Int)((double)translateDiffX*speedX)+translateStartX;
          y=(Int)((double)translateDiffY*speedY)+translateStartY;
          break;
        case GraphicTranslateAnimationTypeAccelerated:
          accelX=translateDiffX/(((double)duration)*((double)duration)/4.0);
          accelY=translateDiffY/(((double)duration)*((double)duration)/4.0);
          speedX=accelX*((double)duration)/2;
          speedY=accelY*((double)duration)/2;
          if (elapsedTime<=duration/2) {
            x=(Int)(translateStartX+accelX*((double)elapsedTime)*((double)elapsedTime)/2);
            y=(Int)(translateStartY+accelY*((double)elapsedTime)*((double)elapsedTime)/2);
          } else {
            double t=((double)elapsedTime)-((double)duration)/2;
            x=(Int)(translateStartX+translateDiffX/2+speedX*t-accelX*t*t/2);
            y=(Int)(translateStartY+translateDiffY/2+speedY*t-accelY*t*t/2);
          }
          break;
        default:
          FATAL("animation type is not supported",NULL);
      }
    } else {
      x=translateEndX;
      y=translateEndY;
      translateStartTime=translateEndTime;
    }
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


// Sets the next scale animation step from the sequence
void GraphicPrimitive::setNextScaleAnimationStep() {
  if (scaleAnimationSequence.size()>0) {
    GraphicScaleAnimationParameter parameter = scaleAnimationSequence.front();
    scaleAnimationSequence.pop_front();
    setScaleAnimation(parameter.getStartTime(),parameter.getStartFactor(),parameter.getEndFactor(),parameter.getInfinite(),parameter.getDuration());
  }
}


// Sets the next translate animation step from the sequence
void GraphicPrimitive::setNextTranslateAnimationStep() {
  if (translateAnimationSequence.size()>0) {
    GraphicTranslateAnimationParameter parameter = translateAnimationSequence.front();
    translateAnimationSequence.pop_front();
    setTranslateAnimation(parameter.getStartTime(),parameter.getStartX(),parameter.getStartY(),parameter.getEndX(),parameter.getEndY(),parameter.getInfinite(),parameter.getDuration(),parameter.getAnimationType());
  }
}

// Sets the next rotate animation step from the sequence
void GraphicPrimitive::setNextRotateAnimationStep() {
  if (rotateAnimationSequence.size()>0) {
    GraphicRotateAnimationParameter parameter = rotateAnimationSequence.front();
    rotateAnimationSequence.pop_front();
    setRotateAnimation(parameter.getStartTime(),parameter.getStartAngle(),parameter.getEndAngle(),parameter.getInfinite(),parameter.getDuration(),parameter.getAnimationType());
  }
}

// Sets the next fade animation step from the sequence
void GraphicPrimitive::setNextFadeAnimationStep() {
  if (fadeAnimationSequence.size()>0) {
    GraphicFadeAnimationParameter parameter = fadeAnimationSequence.front();
    fadeAnimationSequence.pop_front();
    setFadeAnimation(parameter.getStartTime(),parameter.getStartColor(),parameter.getEndColor(),parameter.getInfinite(),parameter.getDuration());
  }
}

// Sets the next texture animation step from the sequence
void GraphicPrimitive::setNextTextureAnimationStep() {
  if (textureAnimationSequence.size()>0) {
    GraphicTextureAnimationParameter parameter = textureAnimationSequence.front();
    textureAnimationSequence.pop_front();
    setTextureAnimation(parameter.getStartTime(),parameter.getStartTexture(),parameter.getEndTexture(),parameter.getInfinite(),parameter.getDuration());
  }
}

// Sets a scale animation sequence
void GraphicPrimitive::setScaleAnimationSequence(std::list<GraphicScaleAnimationParameter> scaleAnimationSequence) {
  this->scaleAnimationSequence=scaleAnimationSequence;
  setNextScaleAnimationStep();
}


// Sets a translate animation sequence
void GraphicPrimitive::setTranslateAnimationSequence(std::list<GraphicTranslateAnimationParameter> translateAnimationSequence) {
  this->translateAnimationSequence=translateAnimationSequence;
  setNextTranslateAnimationStep();
}

// Sets a rotate animation sequence
void GraphicPrimitive::setRotateAnimationSequence(std::list<GraphicRotateAnimationParameter> rotateAnimationSequence) {
  this->rotateAnimationSequence=rotateAnimationSequence;
  setNextRotateAnimationStep();
}

// Sets a fade animation sequence
void GraphicPrimitive::setFadeAnimationSequence(std::list<GraphicFadeAnimationParameter> fadeAnimationSequence) {
  this->fadeAnimationSequence=fadeAnimationSequence;
  setNextFadeAnimationStep();
}

// Sets a texture animation sequence
void GraphicPrimitive::setTextureAnimationSequence(std::list<GraphicTextureAnimationParameter> textureAnimationSequence) {
  this->textureAnimationSequence=textureAnimationSequence;
  setNextTextureAnimationStep();
}

}
