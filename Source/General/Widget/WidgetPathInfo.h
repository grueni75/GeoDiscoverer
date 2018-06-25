//============================================================================
// Name        : WidgetPathInfo.h
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

#ifndef WIDGETPATHINFO_H_
#define WIDGETPATHINFO_H_

namespace GEODISCOVERER {

class WidgetPathInfo: public GEODISCOVERER::WidgetPrimitive {

  // Graphic showing the current location
  GraphicRectangle locationIcon;
  bool hideLocationIcon;

  // Graphic showing the navigation point
  GraphicRectangle navigationPointIcon;

  // Graphical representation of the strings
  FontString *pathNameFontString;
  FontString *pathLengthFontString;
  FontString *pathAltitudeUpFontString;
  FontString *pathAltitudeDownFontString;
  FontString *pathDurationFontString;
  FontString *noAltitudeProfileFontString;
  FontString **altitudeProfileXTickFontStrings;
  FontString **altitudeProfileYTickFontStrings;

  // Holds the points of the altitude profile
  GraphicPointBuffer *altitudeProfileFillPointBuffer;
  GraphicPointBuffer *altitudeProfileLinePointBuffer;
  GraphicPointBuffer *altitudeProfileAxisPointBuffer;

  // Holds the navigation points
  GraphicRectangleList *altitudeProfileNavigationPoints;

  // Color the altitude profile
  GraphicColor altitudeProfileFillColor;
  GraphicColor altitudeProfileLineColor;
  GraphicColor altitudeProfileAxisColor;

  // Minimum distance from nearest path point such that location is off path
  double minDistanceToBeOffRoute;

  // Position information
  Int pathNameWidth;
  Int pathNameOffsetX;
  Int pathNameOffsetY;
  Int pathValuesWidth;
  Int pathLengthOffsetX;
  Int pathLengthOffsetY;
  Int pathAltitudeUpOffsetX;
  Int pathAltitudeUpOffsetY;
  Int pathAltitudeDownOffsetX;
  Int pathAltitudeDownOffsetY;
  Int pathDurationOffsetX;
  Int pathDurationOffsetY;
  Int altitudeProfileWidth;
  Int altitudeProfileHeightWithoutNavigationPoints;
  Int altitudeProfileHeightWithNavigationPoints;
  Int altitudeProfileOffsetX;
  Int altitudeProfileOffsetY;
  Int altitudeProfileLineWidth;
  Int altitudeProfileAxisLineWidth;
  Int altitudeProfileXTickLabelOffsetY;
  Int altitudeProfileYTickLabelOffsetX;
  Int altitudeProfileXTickLabelWidth;
  Int altitudeProfileYTickLabelWidth;
  Int noAltitudeProfileOffsetX;
  Int noAltitudeProfileOffsetY;

  // Number of ticks for the axis of the alttude profile
  Int altitudeProfileXTickCount;
  Int altitudeProfileYTickCount;

  // Minimum altitude difference to use when drawing an altitude profile
  double altitudeProfileMinAltitudeDiff;

  // Currently shown path
  NavigationPath *currentPath;
  static std::string currentPathName;
  static bool currentPathLocked;

  // Last known location
  MapPosition locationPos;

  // Indicates that a redraw of the widget is required
  bool redrawRequired;

  // Start and end of the currently shown altitude part
  Int startIndex;
  Int endIndex;
  Int maxEndIndex;
  double indexLen;
  Int prevX;
  bool firstTouchDown;

  // Ensures that the complete path becomes visible
  void resetPathVisibility(bool widgetVisible);

  // Variables for the visualization thread
  bool quitWidgetPathInfoThread;
  ThreadSignalInfo *updateVisualizationSignal;
  ThreadInfo *widgetPathInfoThreadInfo;
  ThreadMutexInfo *visualizationMutex;
  ThreadMutexInfo *widgetPathInfoThreadWorkingMutex;
  std::string *visualizationPathName;
  std::string *visualizationPathLength;
  std::string *visualizationPathAltitudeUp;
  std::string *visualizationPathAltitudeDown;
  std::string *visualizationPathDuration;
  std::list<GraphicPoint> *visualizationAltitudeProfileFillPoints;
  std::list<GraphicPoint> *visualizationAltitudeProfileLinePoints;
  GraphicPoint *visualizationAltitudeProfileLocationIconPoint;
  bool visualizationAltitudeProfileHideLocationIcon;
  std::list<GraphicPoint> *visualizationAltitudeProfileAxisPoints;
  std::vector<std::string> *visualizationAltitudeProfileXTickLabels;
  std::vector<GraphicPoint> *visualizationAltitudeProfileXTickPoints;
  std::vector<std::string> *visualizationAltitudeProfileYTickLabels;
  std::vector<GraphicPoint> *visualizationAltitudeProfileYTickPoints;
  std::list<NavigationPoint> *visualizationAltitudeProfileNavigationPoints;
  bool visualizationNoAltitudeProfile;
  bool visualizationValid;

public:

  // Constructors and destructor
  WidgetPathInfo(WidgetPage *widgetPage);
  virtual ~WidgetPathInfo();

  // Let the widget work
  virtual bool work(TimestampInMicroseconds t);

  // Called when the widget must be drawn
  virtual void draw(TimestampInMicroseconds t);

  // Called when the map has changed
  virtual void onMapChange(bool widgetVisible, MapPosition pos);

  // Called when the location has changed
  virtual void onLocationChange(bool widgetVisible, MapPosition pos);

  // Called when a path has changed
  virtual void onPathChange(bool widgetVisible, NavigationPath *path, NavigationPathChangeType changeType);

  // Called when a two fingure gesture is done on the widget
  virtual void onTwoFingerGesture(TimestampInMicroseconds t, Int dX, Int dY, double angleDiff, double scaleDiff);

  // Called when the widget is touched
  virtual void onTouchDown(TimestampInMicroseconds t, Int x, Int y);

  // Called when the widget is not touched anymore
  virtual void onTouchUp(TimestampInMicroseconds t, Int x, Int y, bool cancel);

  // Called when some data has changed
  virtual void onDataChange();

  // Recomputes the visualization of the path info
  void updateVisualization();

  // Called when the widget has changed his position
  virtual void updatePosition(Int x, Int y, Int z);

  // Getters and setters
  void setPathNameOffsetX(Int pathNameOffsetX) {
    this->pathNameOffsetX = pathNameOffsetX;
  }

  void setPathNameOffsetY(Int pathNameOffsetY) {
    this->pathNameOffsetY = pathNameOffsetY;
  }

  void setPathNameWidth(Int pathNameWidth) {
    this->pathNameWidth = pathNameWidth;
  }

  void setPathAltitudeDownOffsetY(Int pathAltitudeDownOffsetY) {
    this->pathAltitudeDownOffsetY = pathAltitudeDownOffsetY;
  }

  void setPathAltitudeUpOffsetY(Int pathAltitudeUpOffsetY) {
    this->pathAltitudeUpOffsetY = pathAltitudeUpOffsetY;
  }

  void setPathDurationOffsetY(Int pathDurationOffsetY) {
    this->pathDurationOffsetY = pathDurationOffsetY;
  }

  void setPathLengthOffsetY(Int pathLengthOffsetY) {
    this->pathLengthOffsetY = pathLengthOffsetY;
  }

  void setPathValuesWidth(Int pathValuesWidth) {
    this->pathValuesWidth = pathValuesWidth;
  }

  void setAltitudeProfileAxisColor(
      const GraphicColor& altitudeProfileAxisColor) {
    this->altitudeProfileAxisColor = altitudeProfileAxisColor;
  }

  void setAltitudeProfileFillColor(
      const GraphicColor& altitudeProfileFillColor) {
    this->altitudeProfileFillColor = altitudeProfileFillColor;
  }

  void setAltitudeProfileLineColor(
      const GraphicColor& altitudeProfileLineColor) {
    this->altitudeProfileLineColor = altitudeProfileLineColor;
  }

  void setAltitudeProfileLineWidth(Int altitudeProfileLineWidth) {
    this->altitudeProfileLineWidth = altitudeProfileLineWidth;
  }

  void setAltitudeProfileOffsetX(Int altitudeProfileOffsetX) {
    this->altitudeProfileOffsetX = altitudeProfileOffsetX;
  }

  void setAltitudeProfileOffsetY(Int altitudeProfileOffsetY) {
    this->altitudeProfileOffsetY = altitudeProfileOffsetY;
  }

  void setNoAltitudeProfileOffsetX(Int noAltitudeProfileOffsetX) {
    this->noAltitudeProfileOffsetX = noAltitudeProfileOffsetX;
  }

  void setNoAltitudeProfileOffsetY(Int noAltitudeProfileOffsetY) {
    this->noAltitudeProfileOffsetY = noAltitudeProfileOffsetY;
  }

  void setAltitudeProfileHeightWithoutNavigationPoints(Int altitudeProfileHeightWithoutNavigationPoints) {
    this->altitudeProfileHeightWithoutNavigationPoints = altitudeProfileHeightWithoutNavigationPoints;
  }

  void setAltitudeProfileHeightWithNavigationPoints(Int altitudeProfileHeightWithNavigationPoints) {
    this->altitudeProfileHeightWithNavigationPoints = altitudeProfileHeightWithNavigationPoints;
  }

  void setAltitudeProfileWidth(Int altitudeProfileWidth) {
    this->altitudeProfileWidth = altitudeProfileWidth;
  }

  void setAltitudeProfileMinAltitudeDiff(double altitudeProfileMinAltitudeDiff) {
    this->altitudeProfileMinAltitudeDiff = altitudeProfileMinAltitudeDiff;
  }

  void setAltitudeProfileXTickCount(Int altitudeProfileXTickCount);

  void setAltitudeProfileYTickCount(Int altitudeProfileYTickCount);

  void setAltitudeProfileXTickLabelOffsetY(
      Int altitudeProfileXTickLabelOffsetY) {
    this->altitudeProfileXTickLabelOffsetY = altitudeProfileXTickLabelOffsetY;
  }

  void setAltitudeProfileYTickLabelOffsetX(
      Int altitudeProfileYTickLabelOffsetX) {
    this->altitudeProfileYTickLabelOffsetX = altitudeProfileYTickLabelOffsetX;
  }

  void setAltitudeProfileAxisLineWidth(Int altitudeProfileAxisLineWidth) {
    this->altitudeProfileAxisLineWidth = altitudeProfileAxisLineWidth;
  }

  GraphicRectangle *getLocationIcon() {
    return &locationIcon;
  }

  GraphicRectangle *getNavigationPointIcon() {
    return &navigationPointIcon;
  }

  void setAltitudeProfileXTickLabelWidth(Int altitudeProfileXTickLabelWidth) {
    this->altitudeProfileXTickLabelWidth = altitudeProfileXTickLabelWidth;
  }

  void setAltitudeProfileYTickLabelWidth(Int altitudeProfileYTickLabelWidth) {
    this->altitudeProfileYTickLabelWidth = altitudeProfileYTickLabelWidth;
  }

  static void setCurrentPathLocked(bool currentPathLocked, const char *file, int line) {
    WidgetPathInfo::currentPathLocked = currentPathLocked;
    core->getConfigStore()->setIntValue("Navigation","pathInfoLocked",currentPathLocked,file,line);
  }

  void setPathAltitudeDownOffsetX(Int pathAltitudeDownOffsetX) {
    this->pathAltitudeDownOffsetX = pathAltitudeDownOffsetX;
  }

  void setPathAltitudeUpOffsetX(Int pathAltitudeUpOffsetX) {
    this->pathAltitudeUpOffsetX = pathAltitudeUpOffsetX;
  }

  void setPathDurationOffsetX(Int pathDurationOffsetX) {
    this->pathDurationOffsetX = pathDurationOffsetX;
  }

  void setPathLengthOffsetX(Int pathLengthOffsetX) {
    this->pathLengthOffsetX = pathLengthOffsetX;
  }

};

} /* namespace GEODISCOVERER */
#endif /* WIDGETPATHINFO_H_ */
