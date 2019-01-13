//============================================================================
// Name        : WidgetNavigation.h
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


#ifndef WIDGETNAVIGATION_H_
#define WIDGETNAVIGATION_H_

namespace GEODISCOVERER {

class WidgetNavigation : public WidgetPrimitive {

protected:

  // Update interval
  TimestampInMicroseconds updateInterval;

  // Color if the widget is not busy
  GraphicColor normalColor;

  // Color if the widget is busy
  GraphicColor busyColor;

  // Backup of the inactive color
  GraphicColor inactiveColorBackup;

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

  // Skip the current active turn
  bool skipTurn;

  // Decides which info is displayed in the second row
  Int secondRowState;

  // Indicates if widget is active
  bool active;

  // Current navigation metrics
  double locationBearingNominal,locationBearingActual;

  // Vertical positions of the different texts
  Int textColumnCount;
  Int textRowFirstOffsetY;
  Int textRowSecondOffsetY;
  Int textRowThirdOffsetY;
  Int textRowFourthOffsetY;
  Int textColumnOffsetX;
  Int turnDistanceValueOffsetY;
  Int clockOffsetY;

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
  FontString *turnFontString;
  FontString *durationLabelFontString;
  FontString *durationValueFontString;
  FontString *clockFontString;
  FontString *altitudeLabelFontString;
  FontString *altitudeValueFontString;
  FontString *trackLengthLabelFontString;
  FontString *trackLengthValueFontString;
  FontString *speedLabelFontString;
  FontString *speedValueFontString;
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

  // Indicates that this is the first time the widget runs
  bool firstRun;

  // Last update of the clock
  TimestampInSeconds lastClockUpdate;

  // Indicates that the widget runs on a watch
  bool isWatch;

  // Min radius a touch has to have from the center to register as pan command
  double minPanDetectionRadius;

  // Speed at which the panning shall be performed
  double panSpeed;

  // Controls the panning of the map
  bool panActive;
  TimestampInMicroseconds panStartTime;
  double panAngle;
  Int panXInt, panYInt;
  double panXDouble, panYDouble;

  // Indicates if the remote server is active
  bool remoteServerActive;

  // Updates various flags
  virtual void updateFlags(Int x, Int y);

public:

  // Constructor
  WidgetNavigation(WidgetPage *widgetPage);

  // Destructor
  virtual ~WidgetNavigation();

  // Let the widget work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget is not touched anymore
  virtual void onTouchUp(TimestampInMicroseconds t, Int x, Int y, bool cancel);

  // Called when the widget is touched
  virtual void onTouchDown(TimestampInMicroseconds t, Int x, Int y);

  // Called when the widget must be drawn
  virtual void draw(TimestampInMicroseconds t);

  // Called when the widget has changed his position
  virtual void updatePosition(Int x, Int y, Int z);

  // Getters and setters
  void setUpdateInterval(TimestampInMicroseconds updateInterval) {
    this->updateInterval=updateInterval;
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

  void setTextColumnOffsetX(Int textColumnOffsetX) {
    this->textColumnOffsetX = textColumnOffsetX;
  }

  void setTextRowFirstOffsetY(Int textRowFirstOffsetY) {
    this->textRowFirstOffsetY = textRowFirstOffsetY;
  }

  void setTextRowFourthOffsetY(Int textRowFourthOffsetY) {
    this->textRowFourthOffsetY = textRowFourthOffsetY;
  }

  void setTextRowSecondOffsetY(Int textRowSecondOffsetY) {
    this->textRowSecondOffsetY = textRowSecondOffsetY;
  }

  void setTextRowThirdOffsetY(Int textRowThirdOffsetY) {
    this->textRowThirdOffsetY = textRowThirdOffsetY;
  }

  void setTextColumnCount(Int textColumnCount) {
    this->textColumnCount = textColumnCount;
  }

  void setClockOffsetY(Int clockOffsetY) {
    this->clockOffsetY = clockOffsetY;
  }

  void setMinPanDetectionRadius(double minPanDetectionRadius) {
    this->minPanDetectionRadius = minPanDetectionRadius;
  }

  void setPanSpeed(double panSpeed) {
    this->panSpeed = panSpeed;
  }

  void setBusyColor(GraphicColor busyColor) {
    this->normalColor = activeColor;
    this->busyColor = busyColor;
  }

};

}

#endif /* WIDGETNAVIGATION_H_ */
