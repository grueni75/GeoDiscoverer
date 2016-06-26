//============================================================================
// Name        : WidgetButton.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#include <Core.h>

namespace GEODISCOVERER {

// Constructor
WidgetButton::WidgetButton(WidgetPage *widgetPage) : WidgetPrimitive(widgetPage) {
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
      nextDispatchTime=t+widgetPage->getWidgetEngine()->getButtonRepeatDelay();
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
      nextDispatchTime=t+widgetPage->getWidgetEngine()->getButtonRepeatPeriod();
    }

  }
  return changed;
}

// Executed if the widget has been untouched
void WidgetButton::onTouchUp(TimestampInMicroseconds t, Int x, Int y, bool cancel) {
  WidgetPrimitive::onTouchUp(t,x,y,cancel);
  if (getIsHit()) {

    // Execute the command only if the repeating dispatching has not yet started
    if ((!repeat)||(t<nextDispatchTime)) {
      core->getCommander()->execute(command);
    }
  }
}


}
