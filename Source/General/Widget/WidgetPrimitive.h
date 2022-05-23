//============================================================================
// Name        : WidgetBase.h
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

#include <GraphicRectangle.h>

#ifndef WIDGETPRIMITIVE_H_
#define WIDGETPRIMITIVE_H_

namespace GEODISCOVERER {

typedef enum {WidgetTypePrimitive, WidgetTypeButton, WidgetTypeCheckbox, WidgetTypeMeter, WidgetTypeScale, WidgetTypeStatus, WidgetTypeCursorInfo, WidgetTypeNavigation, WidgetTypePathInfo, WidgetTypeAddressPoint, WidgetTypeEBike, WidgetTypeForumslader} WidgetType;

class WidgetContainer;
class WidgetPage;

class WidgetPrimitive : public GraphicRectangle {

protected:

  WidgetType widgetType;              // Type of widget
  WidgetContainer *widgetContainer;   // Parent widget page
  GraphicColor activeColor;           // Color if the widget is active
  GraphicColor inactiveColor;         // Color if the widget is inactive
  bool isFirstTimeSelected;           // Indicates that the widget has just been selected
  bool isHit;                         // Indicates that the widget is hit by the pointer
  bool isSelected;                    // Indicates that the widget is selected
  bool isHidden;                      // Indicates that the widget shall not be activated
  Int xHidden;                        // X coordinate when widget is outside screen
  Int yHidden;                        // Y coordinate when widget is outside screen
  Int xOriginal;                      // Original x coordinate of the widget when it was on screen
  Int yOriginal;                      // Original y coordinate of the widget when it was on screen
  Int touchMode;                      // Indicates if touch mode is enabled or disabled

  // Updates various flags
  virtual void updateFlags(Int x, Int y);

public:

  // Constructors and destructor
  WidgetPrimitive(WidgetContainer *widgetContainer);
  virtual ~WidgetPrimitive();

  // Called when a two fingure gesture is done on the widget
  virtual void onTwoFingerGesture(TimestampInMicroseconds t, Int dX, Int dY, double angleDiff, double scaleDiff);

  // Called when the widget is touched
  virtual void onTouchDown(TimestampInMicroseconds t, Int x, Int y);

  // Called when the widget is not touched anymore
  virtual void onTouchUp(TimestampInMicroseconds t, Int x, Int y, bool cancel);

  // Called when the map has changed
  virtual void onMapChange(bool widgetVisible, MapPosition pos);

  // Called when the location changes
  virtual void onLocationChange(bool widgetVisible, MapPosition pos);

  // Called when a path changes
  virtual void onPathChange(bool widgetVisible, NavigationPath *path, NavigationPathChangeType changeType);

  // Called when some data has changed
  virtual void onDataChange();

  // Called when touch mode is changed
  void setTouchMode(Int touchMode) {
    this->touchMode=touchMode;
  }

  // Called when the widget has changed his position
  virtual void updatePosition(Int x, Int y, Int z);

  // Called when the widget must be drawn
  virtual void draw(TimestampInMicroseconds t);

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

  virtual void setIsHidden(bool isHidden) {
    this->isHidden = isHidden;
  }

  Int getXHidden() const {
    return xHidden;
  }

  Int getYHidden() const {
    return yHidden;
  }

  void setXHidden(Int xHidden) {
    this->xOriginal = x;
    this->xHidden = xHidden;
  }

  void setYHidden(Int yHidden) {
    this->yOriginal = y;
    this->yHidden = yHidden;
  }

  Int getXOriginal() const {
    return xOriginal;
  }

  Int getYOriginal() const {
    return yOriginal;
  }
};

}

#endif /* WIDGETPRIMITIVE_H_ */
