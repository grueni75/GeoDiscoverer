//============================================================================
// Name        : WidgetBase.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef WIDGETPRIMITIVE_H_
#define WIDGETPRIMITIVE_H_

namespace GEODISCOVERER {

typedef enum {WidgetTypePrimitive, WidgetTypeButton, WidgetTypeCheckbox, WidgetTypeMeter, WidgetTypeScale, WidgetTypeStatus, WidgetTypeNavigation, WidgetTypePathInfo} WidgetType;

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

  // Called when a two fingure gesture is done on the widget
  virtual void onTwoFingerGesture(TimestampInMicroseconds t, Int dX, Int dY, double angleDiff, double scaleDiff);

  // Called when the widget is touched
  virtual void onTouchDown(TimestampInMicroseconds t, Int x, Int y);

  // Called when the widget is not touched anymore
  virtual void onTouchUp(TimestampInMicroseconds t, Int x, Int y);

  // Called when the map has changed
  virtual void onMapChange(bool widgetVisible, MapPosition pos);

  // Called when the location changes
  virtual void onLocationChange(bool widgetVisible, MapPosition pos);

  // Called when a path changes
  virtual void onPathChange(bool widgetVisible, NavigationPath *path, NavigationPathChangeType changeType);

  // Called when the widget has changed his position
  virtual void updatePosition(Int x, Int y, Int z);

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
