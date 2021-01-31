//============================================================================
// Name        : WidgetBase.cpp
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
#include <WidgetPrimitive.h>
#include <Screen.h>
#include <WidgetContainer.h>
#include <MapPosition.h>

namespace GEODISCOVERER {

// Constructor
WidgetPrimitive::WidgetPrimitive(WidgetContainer *widgetContainer) : GraphicRectangle(widgetContainer->getScreen()) {
  type=GraphicTypeWidget;
  this->widgetContainer=widgetContainer;
  widgetType=WidgetTypePrimitive;
  isHit=false;
  isSelected=false;
  isFirstTimeSelected=false;
  isHidden=false;
  xHidden=0;
  yHidden=0;
  xOriginal=0;
  yOriginal=0;
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

// Called when some data has changed
void WidgetPrimitive::onDataChange() {

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

// Called when the rectangle must be drawn
void WidgetPrimitive::draw(TimestampInMicroseconds t) {

  // Do not draw if primitive is hidden
  if (getIsHidden()) 
    return;

  // Don't draw if scale factor is 0
  if (scale==0)
    return;

  // Set color
  screen->setColor(getColor().getRed(),getColor().getGreen(),getColor().getBlue(),getColor().getAlpha());

  // Draw widget
  screen->startObject();
  screen->translate(getX()+getIconWidth()/2,getY()+getIconHeight()/2,0);
  screen->scale(scale,scale,1.0);
  Int x1=-getIconWidth()/2;
  Int y1=-getIconHeight()/2;
  Int x2=x1+getWidth();
  Int y2=y1+getHeight();
  screen->drawRectangle(x1,y1,x2,y2,getTexture(),getFilled());
  screen->endObject();
}

}
