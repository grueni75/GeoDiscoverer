//============================================================================
// Name        : WidgetButton.cpp
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
WidgetButton::WidgetButton() : WidgetPrimitive() {
  widgetType=WidgetTypeButton;
  repeat=true;
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
      nextDispatchTime=t+core->getWidgetEngine()->getButtonRepeatDelay();
    }

  }
}

// Executed every time the graphic engine needs to draw
bool WidgetButton::work(TimestampInMicroseconds t) {

  // Do the inherited stuff
  bool changed=WidgetPrimitive::work(t);

  // Handle button presses
  if (getIsSelected()) {

    // Repeat the command if the initial delay is over
    if ((repeat)&&(t>=nextDispatchTime)) {
      core->getCommander()->execute(command);
      nextDispatchTime=t+core->getWidgetEngine()->getButtonRepeatPeriod();
    }

  }
  return changed;
}

// Executed if the widget has been untouched
void WidgetButton::onTouchUp(TimestampInMicroseconds t, Int x, Int y) {
  WidgetPrimitive::onTouchUp(t,x,y);
  if (getIsHit()) {

    // Execute the command only if the repeating dispatching has not yet started
    if ((!repeat)||(t<nextDispatchTime)) {
      core->getCommander()->execute(command);
    }
  }
}


}
