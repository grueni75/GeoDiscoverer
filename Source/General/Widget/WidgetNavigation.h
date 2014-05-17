//============================================================================
// Name        : WidgetNavigation.h
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


#ifndef WIDGETNAVIGATION_H_
#define WIDGETNAVIGATION_H_

namespace GEODISCOVERER {

class WidgetNavigation : public WidgetPrimitive {

protected:

  // Update interval
  TimestampInMicroseconds updateInterval;

  // Graphic showing the direction
  GraphicRectangle directionIcon;

  // Graphic showing the target
  GraphicRectangle targetIcon;

  // Graphic showing the separator
  GraphicRectangle separatorIcon;

  // Container that represents the target (displacement)
  GraphicObject targetObject;

  // Container that represents the direction plus target
  GraphicObject compassObject;

  // Next timestamp when to update the widget
  TimestampInMicroseconds nextUpdateTime;

  // Decides if the compass is shown
  bool hideCompass;

  // Decides if the target is shown
  bool hideTarget;

  // Show the turn
  bool showTurn;

  // Current navigation metrics
  double locationBearingNominal,locationBearingActual;

  // Vertical positions of the different texts
  Int durationLabelOffsetY;
  Int durationValueOffsetY;
  Int targetDistanceLabelOffsetY;
  Int targetDistanceValueOffsetY;
  Int turnDistanceValueOffsetY;

  // Color of the turn
  GraphicColor turnColor;

  // Definition of the turn arrow
  Int turnLineWidth;
  Int turnLineArrowOverhang;
  Int turnLineArrowHeight;
  Int turnLineStartHeight;
  Int turnLineMiddleHeight;
  Int turnLineStartX;
  Int turnLineStartY;

  // Font string objects for drawing
  FontString *distanceLabelFontString;
  FontString *distanceValueFontString;
  FontString *durationLabelFontString;
  FontString *durationValueFontString;
  FontString *orientationLabelFontStrings[4];

  // Holds the points of the arrow
  GraphicPointBuffer turnArrowPointBuffer;

  // Duration of a direction change in milliseconds
  double directionChangeDuration;

  // Radius of the target icon
  double targetRadius;

  // Radius of the orientation labels
  double orientationLabelRadius;

  // Last navigation infos
  NavigationInfo prevNavigationInfo;

public:

  // Constructor
  WidgetNavigation();

  // Destructor
  virtual ~WidgetNavigation();

  // Let the widget work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget must be drawn
  virtual void draw(Screen *screen, TimestampInMicroseconds t);

  // Called when the widget has changed his position
  virtual void updatePosition(Int x, Int y, Int z);

  // Getters and setters
  void setUpdateInterval(TimestampInMicroseconds updateInterval) {
    this->updateInterval=updateInterval;
  }

   void setDurationLabelOffsetY(Int durationLabelOffsetY) {
    this->durationLabelOffsetY = durationLabelOffsetY;
  }

  void setDurationValueOffsetY(Int durationValueOffsetY) {
    this->durationValueOffsetY = durationValueOffsetY;
  }

  GraphicRectangle *getDirectionIcon() {
    return &directionIcon;
  }

  GraphicRectangle *getTargetIcon() {
    return &targetIcon;
  }

  GraphicRectangle *getSeparatorIcon() {
    return &separatorIcon;
  }

  void setDirectionChangeDuration(double directionChangeDuration) {
    this->directionChangeDuration = directionChangeDuration;
  }

  void setTargetRadius(double targetRadius) {
    this->targetRadius = targetRadius;
  }

  void setOrientationLabelRadius(double orientationLabelRadius) {
    this->orientationLabelRadius = orientationLabelRadius;
  }

  void setTargetDistanceLabelOffsetY(Int targetDistanceLabelOffsetY) {
    this->targetDistanceLabelOffsetY = targetDistanceLabelOffsetY;
  }

  void setTargetDistanceValueOffsetY(Int targetDistanceValueOffsetY) {
    this->targetDistanceValueOffsetY = targetDistanceValueOffsetY;
  }

  void setTurnDistanceValueOffsetY(Int turnDistanceValueOffsetY) {
    this->turnDistanceValueOffsetY = turnDistanceValueOffsetY;
  }

  void setTurnLineArrowHeight(Int turnLineArrowHeight) {
    this->turnLineArrowHeight = turnLineArrowHeight;
  }

  void setTurnLineArrowOverhang(Int turnLineArrowOverhang) {
    this->turnLineArrowOverhang = turnLineArrowOverhang;
  }

  void setTurnLineMiddleHeight(Int turnLineMiddleHeight) {
    this->turnLineMiddleHeight = turnLineMiddleHeight;
  }

  void setTurnLineStartHeight(Int turnLineStartHeight) {
    this->turnLineStartHeight = turnLineStartHeight;
  }

  void setTurnLineStartX(Int turnLineStartX) {
    this->turnLineStartX = turnLineStartX;
  }

  void setTurnLineStartY(Int turnLineStartY) {
    this->turnLineStartY = turnLineStartY;
  }

  void setTurnLineWidth(Int turnLineWidth) {
    this->turnLineWidth = turnLineWidth;
  }

  void setTurnColor(const GraphicColor turnColor) {
    this->turnColor = turnColor;
  }
};

}

#endif /* WIDGETNAVIGATION_H_ */
