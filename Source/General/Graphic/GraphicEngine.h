//============================================================================
// Name        : GraphicEngine.h
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

#include <GraphicPosition.h>
#include <GraphicRectangle.h>
#include <GraphicObject.h>

#ifndef GRAPHICENGINE_H_
#define GRAPHICENGINE_H_

namespace GEODISCOVERER {

class GraphicEngine {

protected:

  // Device this engine renders for
  Device *device;

  // Map object
  GraphicObject *map;

  // Navigation points object
  GraphicObject *navigationPoints;

  // Path animators object
  GraphicObject pathAnimators;

  // Center icon
  GraphicRectangle centerIcon;

  // Location indicator icon
  GraphicRectangle locationIcon;

  // Target indicator icon
  GraphicRectangle targetIcon;

  // Navigation point indicator icon
  GraphicRectangle navigationPointIcon;

  // Arrow icon
  GraphicRectangle arrowIcon;

  // Path direction icon
  GraphicRectangle pathDirectionIcon;

  // Path start flag
  GraphicRectangle pathStartFlagIcon;

  // Path end flag
  GraphicRectangle pathEndFlagIcon;

  // Compass cone icon
  GraphicRectangle compassConeIcon;

  // Not cached tile image
  GraphicRectangle tileImageNotCached;

  // Not downloaded tile image
  GraphicRectangle tileImageNotDownloaded;

  // Not downloaded tile image
  GraphicRectangle tileImageDownloadErrorOccured;

  // Radiuses of the accuracy ellipse
  Int locationAccuracyRadiusX;
  Int locationAccuracyRadiusY;

  // Color of the background of the accuracy circle around the location indicator
  GraphicColor locationAccuracyBackgroundColor;

  // Color of the accuracy circle around the location indicator
  GraphicColor locationAccuracyCircleColor;

  // Time in us to wait after a user interaction before the cursor is hidden
  TimestampInMicroseconds centerIconTimeout;

  // Current position
  GraphicPosition pos;

  // Current position mutex
  ThreadMutexInfo *posMutex;

  // Enable debugging mode
  Int debugMode;

  // Drawing mutex
  ThreadMutexInfo *drawingMutex;

  // Previous position
  GraphicPosition previousPosition;

  // Last start time of the center icon fade
  TimestampInMicroseconds lastCenterIconFadeStartTime;

  // Indicates that the engine is drawing
  bool isDrawing;

  // The start time of the last drawing operation
  TimestampInMicroseconds lastDrawingStartTime;

  // Default duration of a fade animation
  TimestampInMicroseconds fadeDuration;

  // Default duration of a blink animation
  TimestampInMicroseconds blinkDuration;

  // Reference DPI the map tiles have been created for
  Int mapReferenceDPI;

  // Statistical infos
  double minDrawingTime;
  double maxDrawingTime;
  double totalDrawingTime;
  double minIdleTime;
  double maxIdleTime;
  double totalIdleTime;
  Int frameCount;
  TimestampInMicroseconds targetDrawingTime;
  bool drawingTooSlow;

public:

  // Constructors and destructor
  GraphicEngine(Device *device);
  virtual ~GraphicEngine();

  // Inits dynamic data
  void init();

  // Clears all graphic
  void destroyGraphic();

  // Creates all graphic
  void createGraphic();

  // Deinits dynamic data
  void deinit();

  // Does the drawing
  bool draw(bool forceRedraw);

  // Outputs statistical infos
  void outputStats();

  // Returns the additional scale to match the scale the map tiles have been made for
  double getMapTileToScreenScale(Screen *screen);

  // Getters and setters
  void lockDrawing(const char *file, int line) const {
    core->getThread()->lockMutex(drawingMutex,file,line);
  }

  void unlockDrawing() const {
    core->getThread()->unlockMutex(drawingMutex);
  }

  TimestampInMicroseconds getBlinkDuration() const {
    return blinkDuration;
  }

  TimestampInMicroseconds getFadeDuration() const {
    return fadeDuration;
  }

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

  void setNavigationPoints(GraphicObject *navigationPoints)
  {
      this->navigationPoints = navigationPoints;
  }

  GraphicObject *lockPathAnimators(const char *file, int line)
  {
      core->getThread()->lockMutex(drawingMutex, file, line);
      return &pathAnimators;
  }
  void unlockPathAnimators()
  {
      core->getThread()->unlockMutex(drawingMutex);
  }

  GraphicPosition *lockPos(const char *file, int line)
  {
      core->getThread()->lockMutex(posMutex, file, line);
      return &pos;
  }
  void unlockPos()
  {
      core->getThread()->unlockMutex(posMutex);
  }

  GraphicRectangle *lockLocationIcon(const char *file, int line)
  {
    core->getThread()->lockMutex(drawingMutex, file, line);
    return &locationIcon;
  }

  void unlockLocationIcon()
  {
    core->getThread()->unlockMutex(drawingMutex);
  }

  GraphicRectangle *lockTargetIcon(const char *file, int line)
  {
    core->getThread()->lockMutex(drawingMutex, file, line);
    return &targetIcon;
  }

  void unlockTargetIcon()
  {
    core->getThread()->unlockMutex(drawingMutex);
  }

  GraphicRectangle *lockArrowIcon(const char *file, int line)
  {
    core->getThread()->lockMutex(drawingMutex, file, line);
    return &arrowIcon;
  }

  void unlockArrowIcon()
  {
    core->getThread()->unlockMutex(drawingMutex);
  }

  GraphicRectangle *lockCompassConeIcon(const char *file, int line)
  {
    core->getThread()->lockMutex(drawingMutex, file, line);
    return &compassConeIcon;
  }

  void unlockCompassConeIcon()
  {
    core->getThread()->unlockMutex(drawingMutex);
  }

  GraphicRectangle *getPathDirectionIcon()
  {
      return &pathDirectionIcon;
  }

  GraphicRectangle *getTargetIcon()
  {
      return &targetIcon;
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

  GraphicRectangle *getNotCachedTileImage()
  {
      return &tileImageNotCached;
  }

  GraphicRectangle *getDownloadErrorOccuredTileImage()
  {
      return &tileImageDownloadErrorOccured;
  }

  GraphicRectangle *getNotDownloadedTileImage()
  {
      return &tileImageNotDownloaded;
  }

  GraphicRectangle *getPathEndFlagIcon() {
    return &pathEndFlagIcon;
  }

  GraphicRectangle *getPathStartFlagIcon() {
    return &pathStartFlagIcon;
  }

  GraphicRectangle *getNavigationPointIcon() {
    return &navigationPointIcon;
  }

  bool getDrawingTooSlow() const {
    return drawingTooSlow;
  }
};

}

#endif /* GRAPHICENGINE_H_ */
