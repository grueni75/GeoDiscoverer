//============================================================================
// Name        : GraphicEngine.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef GRAPHICENGINE_H_
#define GRAPHICENGINE_H_

namespace GEODISCOVERER {

class GraphicEngine {

protected:

  // Device this engine renders for
  Device *device;

  // Map object
  GraphicObject *map;

  // Path animators object
  GraphicObject pathAnimators;

  // Center icon
  GraphicRectangle centerIcon;

  // Location indicator icon
  GraphicRectangle locationIcon;

  // Target indicator icon
  GraphicRectangle targetIcon;

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
  GraphicRectangle tileImageNotCachedImage;

  // Not downloaded tile image
  GraphicRectangle tileImageNotDownloadedFilename;

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

  // Statistical infos
  double minDrawingTime;
  double maxDrawingTime;
  double totalDrawingTime;
  double minIdleTime;
  double maxIdleTime;
  double totalIdleTime;
  Int frameCount;

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
  void draw(bool forceRedraw);

  // Outputs statistical infos
  void outputStats();

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
      return &tileImageNotCachedImage;
  }

  GraphicRectangle *getNotDownloadedTileImage()
  {
      return &tileImageNotDownloadedFilename;
  }

  GraphicRectangle *getPathEndFlagIcon() {
    return &pathEndFlagIcon;
  }

  GraphicRectangle *getPathStartFlagIcon() {
    return &pathStartFlagIcon;
  }
};

}

#endif /* GRAPHICENGINE_H_ */
