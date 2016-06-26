//============================================================================
// Name        : WidgetBase.cpp
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
WidgetPrimitive::WidgetPrimitive(WidgetPage *widgetPage) : GraphicRectangle(widgetPage->getScreen()) {
  type=GraphicTypeWidget;
  this->widgetPage=widgetPage;
  widgetType=WidgetTypePrimitive;
  isHit=false;
  isSelected=false;
  isFirstTimeSelected=false;
  isHidden=false;
}

// Destructor
WidgetPrimitive::~WidgetPrimitive() {
}

// Updates various flags
void WidgetPrimitive::updateFlags(Int x, Int y) {
  if ((!isHidden)&&(x>=getX())&&(x<=getX()+getIconWidth()-1)&&(y>=getY())&&(y<=getY()+getIconHeight()-1)) {
    isHit=true;
  } else {
    isHit=false;
  }
}

// Executed if the widget is currently touched
void WidgetPrimitive::onTouchDown(TimestampInMicroseconds t, Int x, Int y) {
  updateFlags(x,y);
  if (getIsHit()) {
    if (!isSelected) {
      isFirstTimeSelected=true;
    } else {
      isFirstTimeSelected=false;
    }
    isSelected=true;
  } else {
    isSelected=false;
    isFirstTimeSelected=false;
  }
}

// Executed if the widget has been untouched
void WidgetPrimitive::onTouchUp(TimestampInMicroseconds t, Int x, Int y, bool cancel) {
  updateFlags(x,y);
  isSelected=false;
  isFirstTimeSelected=false;
  if (cancel)
    isHit=false;
}

// Called when the map has changed
void WidgetPrimitive::onMapChange(bool widgetVisible, MapPosition pos) {

}

// Called when the location has changed
void WidgetPrimitive::onLocationChange(bool widgetVisible, MapPosition pos) {

}

// Called when a path has changed
void WidgetPrimitive::onPathChange(bool widgetVisible, NavigationPath *path, NavigationPathChangeType changeType) {

}

// Called when a two fingure gesture is done on the widget
void WidgetPrimitive::onTwoFingerGesture(TimestampInMicroseconds t, Int dX, Int dY, double angleDiff, double scaleDiff) {
  return;
}

// Called when the widget has changed his position
void WidgetPrimitive::updatePosition(Int x, Int y, Int z) {
  setX(x);
  setY(y);
  setZ(z);
}

}
