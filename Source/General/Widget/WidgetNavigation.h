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

  // Graphic showing the target
  GraphicRectangle navigationPointIcon;

  // Graphic showing the arrow
  GraphicRectangle arrowIcon;

  // Graphic showing the separator
  GraphicRectangle separatorIcon;

  // Graphic showing the blind background
  GraphicRectangle blindIcon;

  // Graphic showing the status background
  GraphicRectangle statusIcon;

  // Graphic showing the battery state
  GraphicRectangle batteryIconEmpty;
  GraphicRectangle batteryIconFull;
  GraphicRectangle batteryIconCharging;

  // Container that represents the target (displacement)
  GraphicObject targetObject;

  // Container that represents the navigation point
  GraphicObject navigationPointObject;

  // Container that represents the direction plus target
  GraphicObject compassObject;

  // Width limit for the status texts on watch
  Int statusTextWidthLimit;

  // Radius of the status texts on watch
  Int statusTextRadius;

  // Angle offset of the status texts on watch
  double statusTextAngleOffset;

  // Radius of the clock on watch
  Int clockRadius;

  // Decides if the compass is shown
  bool hideCompass;

  // Decides if the target is shown
  bool hideTarget;

  // Decides if the navigation point is shown
  bool hideNavigationPoint;

  // Decides if the arrow is shown
  bool hideArrow;

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

  // Indicates that the button in the north of the widget is hit
  bool northButtonHit;

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
  FontString *orientationLabelFontStrings[8];

  // Circular strips for drawing on watches
  GraphicCircularStrip *circularStrip[4];
  GraphicCircularStrip clockCircularStrip;

  // Holds the points of the arrow
  GraphicPointBuffer turnArrowPointBuffer;

  // Duration of a direction change in milliseconds
  double directionChangeDuration;

  // Radius of the target icon
  double targetRadius;
  
  // Radius of the orientation labels
  double orientationLabelRadius;

  // Radius of the battery icons
  double batteryIconRadius;

  // Last navigation infos
  NavigationInfo prevNavigationInfo;

  // Indicates that this is the first time the widget runs
  bool firstRun;

  // Last update of the clock
  TimestampInSeconds lastClockUpdate;

  // Indicates that the widget runs on a watch
  bool isWatch;

  // Min radius a touch has to have from the center to register as touch command
  double minTouchDetectionRadius;

  // Opening angle in the south of the widget that is detected as a widget activation
  double circularButtonAngle;

  // Speed at which the panning shall be performed
  double panSpeed;

  // Indicates if the remote server is active
  bool remoteServerActive;

  // Indicates if the widget has been touched the first time after beeing inactive
  bool firstTouchAfterInactive;

  // Indicates if the status text shall be drawn above or below the target and arrow
  bool statusTextAbove;

  // Number of battery dots to show
  int batteryDotCount;

  // Opening angle for the battery indicator
  int batteryTotalAngle;

  // Updates various flags
  virtual void updateFlags(Int x, Int y);

  // Returns the font string to show in the given quadrant
  FontString *getQuadrantFontString(Int i);

  // Draws a compass marker
  void drawCompassMarker(TimestampInMicroseconds t, FontString *marker, double x, double y);

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

  // Draws the status texts on a watch
  void drawStatus(TimestampInMicroseconds t);

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

  GraphicRectangle *getNavigationPointIcon() {
    return &navigationPointIcon;
  }

  GraphicRectangle *getArrowIcon() {
    return &arrowIcon;
  }

  GraphicRectangle *getSeparatorIcon() {
    return &separatorIcon;
  }

  GraphicRectangle *getBlindIcon() {
    return &blindIcon;
  }

  GraphicRectangle *getStatusIcon() {
    return &statusIcon;
  }

  GraphicRectangle *getBatteryIconFull() {
    return &batteryIconFull;
  }

  GraphicRectangle *getBatteryIconEmpty() {
    return &batteryIconEmpty;
  }

  GraphicRectangle *getBatteryIconCharging() {
    return &batteryIconCharging;
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

  void setMinTouchDetectionRadius(double minTouchDetectionRadius) {
    this->minTouchDetectionRadius = minTouchDetectionRadius;
  }

  void setBusyColor(GraphicColor busyColor) {
    this->normalColor = activeColor;
    this->busyColor = busyColor;
  }

  void setStatusTextWidthLimit(Int statusTextWidthLimit) {
    this->statusTextWidthLimit = statusTextWidthLimit;
  }

  void setStatusTextAngleOffset(double statusTextAngleOffset) {
    this->statusTextAngleOffset = statusTextAngleOffset;
  }

  void setStatusTextRadius(Int statusTextRadius) {
    this->statusTextRadius = statusTextRadius;
  }

  void setClockRadius(Int clockRadius) {
    this->clockRadius = clockRadius;
  }

  void setCircularButtonAngle(double circularButtonAngle) {
    this->circularButtonAngle = circularButtonAngle;
  }

  void setBatteryIconRadius(double batteryIconRadius) {
    this->batteryIconRadius = batteryIconRadius;
  }
};

}

#endif /* WIDGETNAVIGATION_H_ */
