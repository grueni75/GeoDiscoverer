//============================================================================
// Name        : WidgetBase.cpp
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
WidgetPrimitive::WidgetPrimitive() : GraphicRectangle() {
  type=GraphicTypeWidget;
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
  if ((x>=getX())&&(x<=getX()+getIconWidth()-1)&&(y>=getY())&&(y<=getY()+getIconHeight()-1)) {
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
void WidgetPrimitive::onTouchUp(TimestampInMicroseconds t, Int x, Int y) {
  updateFlags(x,y);
  isSelected=false;
  isFirstTimeSelected=false;
}

}
