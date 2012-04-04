//============================================================================
// Name        : WidgetBase.h
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


#ifndef WIDGETPRIMITIVE_H_
#define WIDGETPRIMITIVE_H_

namespace GEODISCOVERER {

typedef enum {WidgetTypePrimitive, WidgetTypeButton, WidgetTypeCheckbox, WidgetTypeMeter, WidgetTypeScale, WidgetTypeStatus} WidgetType;

class WidgetPrimitive : public GraphicRectangle {

protected:

  WidgetType widgetType;       // Type of widget
  GraphicColor activeColor;     // Color if the widget is active
  GraphicColor inactiveColor;   // Color if the widget is inactive
  bool isFirstTimeSelected;     // Indicates that the widget has just been selected
  bool isHit;                   // Indicates that the widget is hit by the pointer
  bool isSelected;              // Indicates that the widget is selected
  bool isHidden;                // Indicates that the widget shall not be activated

  // Updates various flags
  void updateFlags(Int x, Int y);

public:

  // Constructors and destructor
  WidgetPrimitive();
  virtual ~WidgetPrimitive();

  // Called when the widget is touched
  virtual void onTouchDown(TimestampInMicroseconds t, Int x, Int y);

  // Called when the widget is not touched anymore
  virtual void onTouchUp(TimestampInMicroseconds t, Int x, Int y);

  // Getters and setters
  bool getIsHit() const
  {
      return isHit;
  }

  WidgetType getWidgetType() const
  {
      return widgetType;
  }

  void setWidgetType(WidgetType widgetType)
  {
      this->widgetType = widgetType;
  }

  GraphicColor getActiveColor() const
  {
      return activeColor;
  }

  GraphicColor getInactiveColor() const
  {
      return inactiveColor;
  }

  void setActiveColor(GraphicColor activeColor)
  {
      this->activeColor = activeColor;
  }

  void setInactiveColor(GraphicColor inactiveColor)
  {
      this->inactiveColor = inactiveColor;
  }

  bool getIsFirstTimeSelected() const
  {
      return isFirstTimeSelected;
  }

  bool getIsSelected() const
  {
      return isSelected;
  }

  bool getIsHidden() const {
    return isHidden;
  }

  void setIsHidden(bool isHidden) {
    this->isHidden = isHidden;
  }

};

}

#endif /* WIDGETPRIMITIVE_H_ */
