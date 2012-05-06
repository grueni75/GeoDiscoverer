//============================================================================
// Name        : GraphicEngine.h
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


#ifndef GRAPHICENGINE_H_
#define GRAPHICENGINE_H_

namespace GEODISCOVERER {

class GraphicEngine {

protected:

  // Map object
  GraphicObject *map;

  // Path animators object
  GraphicObject pathAnimators;

  // Widget page object
  WidgetPage *widgetPage;

  // Center icon
  GraphicRectangle centerIcon;

  // Location indicator icon
  GraphicRectangle locationIcon;

  // Path direction icon
  GraphicRectangle pathDirectionIcon;

  // Compass cone icon
  GraphicRectangle compassConeIcon;

  // Radiuses of the accuracy ellipse
  Int locationAccuracyRadiusX;
  Int locationAccuracyRadiusY;

  // Color of the background of the accuracy circle around the location indicator
  GraphicColor locationAccuracyBackgroundColor;

  // Color of the accuracy circle around the location indicator
  GraphicColor locationAccuracyCircleColor;

  // Line width of the accuracy circle around the location indicator
  Int locationAccuracyCircleLineWidth;

  // Time in us to wait after a user interaction before the cursor is hidden
  TimestampInMicroseconds centerIconTimeout;

  // Current position
  GraphicPosition pos;

  // Enable debugging mode
  Int debugMode;

  // Mutex for accessing the position
  ThreadMutexInfo *posMutex;

  // Mutex for accessing the location icon
  ThreadMutexInfo *locationIconMutex;

  // Mutex for accessing the compass cone icon
  ThreadMutexInfo *compassConeIconMutex;

  // Mutex for accessing the animators of pathes
  ThreadMutexInfo *pathAnimatorsMutex;

  // Previous position
  GraphicPosition previousPosition;

  // Number of frames without any change
  Int noChangeFrameCount;

  // Last start time of the center icon fade
  TimestampInMicroseconds lastCenterIconFadeStartTime;

  // Indicates that the engine is drawing
  bool isDrawing;

  // The start time of the last drawing operation
  TimestampInMicroseconds lastDrawingStartTime;

  // Statistical infos
  double minDrawingTime;
  double maxDrawingTime;
  double totalDrawingTime;
  double minIdleTime;
  double maxIdleTime;
  double totalIdleTime;
  Int frameCount;
  ThreadMutexInfo *statsMutex;

public:

  // Constructors and destructor
  GraphicEngine();
  virtual ~GraphicEngine();

  // Inits dynamic data
  void init();

  // Recreates all graphic
  void graphicInvalidated();

  // Deinits dynamic data
  void deinit();

  // Does the drawing
  void draw(bool forceRedraw);

  // Outputs statistical infos
  void outputStats();

  // Getters and setters
  bool getIsDrawing() const
  {
      return isDrawing;
  }

  Int getDebugMode() const
  {
      return debugMode;
  }

  void setMap(GraphicObject *map)
  {
      this->map = map;
  }

  GraphicObject *lockPathAnimators()
  {
      core->getThread()->lockMutex(pathAnimatorsMutex);
      return &pathAnimators;
  }
  void unlockPathAnimators()
  {
      core->getThread()->unlockMutex(pathAnimatorsMutex);
  }

  void setWidgetPage(WidgetPage *widgetPage)
  {
      this->widgetPage = widgetPage;
  }
  GraphicPosition *lockPos()
  {
      core->getThread()->lockMutex(posMutex);
      return &pos;
  }
  void unlockPos()
  {
      core->getThread()->unlockMutex(posMutex);
  }

  GraphicRectangle *lockLocationIcon()
  {
    core->getThread()->lockMutex(locationIconMutex);
    return &locationIcon;
  }

  void unlockLocationIcon()
  {
    core->getThread()->unlockMutex(locationIconMutex);
  }


  GraphicRectangle *lockCompassConeIcon()
  {
    core->getThread()->lockMutex(compassConeIconMutex);
    return &compassConeIcon;
  }

  void unlockCompassConeIcon()
  {
    core->getThread()->unlockMutex(compassConeIconMutex);
  }

  GraphicRectangle *getPathDirectionIcon()
  {
      return &pathDirectionIcon;
  }

  Int getLocationAccuracyRadiusX() const
  {
      return locationAccuracyRadiusX;
  }

  Int getLocationAccuracyRadiusY() const
  {
      return locationAccuracyRadiusY;
  }

  void setLocationAccuracyRadiusX(Int locationAccuracyRadiusX)
  {
      this->locationAccuracyRadiusX = locationAccuracyRadiusX;
  }

  void setLocationAccuracyRadiusY(Int locationAccuracyRadiusY)
  {
      this->locationAccuracyRadiusY = locationAccuracyRadiusY;
  }
};

}

#endif /* GRAPHICENGINE_H_ */
