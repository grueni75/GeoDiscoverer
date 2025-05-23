//============================================================================
// Name        : WidgetButton.cpp
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
#include <WidgetButton.h>
#include <WidgetContainer.h>
#include <Commander.h>
#include <WidgetEngine.h>
#include <WidgetContainer.h>
#include <GraphicEngine.h>

namespace GEODISCOVERER {

// Constructor
WidgetButton::WidgetButton(WidgetContainer *widgetContainer) : WidgetPrimitive(widgetContainer) {
  widgetType=WidgetTypeButton;
  longPressCommand="";
  longPressTime=0;
  repeat=true;
  skipCommand=false;
  safetyState=false;
  safetyStateStart=0;
  safetyStateEnd=0;
  safetyStep=false;
}

// Destructor
WidgetButton::~WidgetButton() {
}

// Executed if the widget is currently touched
void WidgetButton::onTouchDown(TimestampInMicroseconds t, Int x, Int y) {
  WidgetPrimitive::onTouchDown(t,x,y);
  if (getIsSelected()) {

    // Compute the next command dispatch time if the button is pressed the first time
    if (getIsFirstTimeSelected()) {
      nextDispatchTime=t+widgetContainer->getWidgetEngine()->getButtonRepeatDelay();
      if (longPressCommand!="")
        longPressTime=t+widgetContainer->getWidgetEngine()->getButtonLongPressDelay();
    }

  }
}

// Executed every time the graphic engine needs to draw
bool WidgetButton::work(TimestampInMicroseconds t) {

  // Do the inherited stuff
  bool changed=WidgetPrimitive::work(t);

  // Handle the color change
  if (safetyState) {
    if ((safetyStateStart!=0)&&(t>=safetyStateStart)) {      
      setFadeAnimation(t,getColor(),safetyStepColor,false,fadeDuration);      
      safetyStateStart=0;
    } else {
      if (t>=safetyStateEnd) {
        setFadeAnimation(t,getColor(),activeColor,false,fadeDuration);
        safetyState=false;
      }
    }
  }
  
  // Handle button presses
  if (getIsSelected()) {

    // Repeat the command if the initial delay is over
    if ((repeat)&&(t>=nextDispatchTime)) {
      widgetContainer->getWidgetEngine()->queueCommand(command,NULL);
      nextDispatchTime=t+widgetContainer->getWidgetEngine()->getButtonRepeatPeriod();
    }

    // Execute the long press command if delay is long enough
    if ((longPressTime!=0)&&(t>=longPressTime)) {
      widgetContainer->getWidgetEngine()->queueCommand(longPressCommand,NULL);
      skipCommand=true;
    }
    
  }
  return changed;
}

// Executed if the widget has been untouched
void WidgetButton::onTouchUp(TimestampInMicroseconds t, Int x, Int y, bool cancel) {
  WidgetPrimitive::onTouchUp(t,x,y,cancel);
  if (getIsHit()) {

    // Frist check if we are allowed to execute
    if (safetyStep) { 
      if (!safetyState) {
        safetyState=true;
        safetyStateStart=t+fadeDuration;
        safetyStateEnd=safetyStateStart+safetyStepDuration;
        return;
      } else {
        safetyState=false;
      }
    }

    // Execute the command only if the repeating dispatching has not yet started
    if ((!skipCommand)&&((!repeat)||(t<nextDispatchTime))) {
      widgetContainer->getWidgetEngine()->queueCommand(command,NULL);
    }
    skipCommand=false;
  }
}

// Executed every time the graphic engine needs to draw
void WidgetButton::draw(TimestampInMicroseconds t) {

  // Get the fade scale depending on ambiet mode
  double fadeScale=widgetContainer->getWidgetEngine()->getGraphicEngine()->getAmbientFadeScale();
  if (fadeScale!=1.0)
    screen->setAlphaScale(fadeScale);
  if (fadeScale>0) {
    WidgetPrimitive::draw(t);
  }
  if (fadeScale!=1.0)
    screen->setAlphaScale(1.0);
}

}