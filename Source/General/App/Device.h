//============================================================================
// Name        : Device.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#ifndef DEVICE_H_
#define DEVICE_H_

namespace GEODISCOVERER {

class Device {

protected:

  // Name of the device
  std::string name;

  // Density of the screen
  Int DPI;

  // Diagonal of the screen
  double diagonal;

  // Indicates if this device is rendering for a remote cockpit app
  bool cockpit;

  // Update period
  TimestampInMicroseconds updatePeriod;

  // Next time to draw
  TimestampInMicroseconds nextUpdateTime;

  // Components of the device
  Screen *screen;
  FontEngine *fontEngine;
  WidgetEngine *widgetEngine;
  GraphicEngine *graphicEngine;

  // Number of frames without any change
  Int noChangeFrameCount;

  // The currently visible pages
  GraphicObject *visibleWidgetPages;

  // Properties of the screen
  GraphicScreenOrientation orientation;
  Int width;
  Int height;

public:

  // Constructor
  Device(std::string name, Int DPI, double diagonal, TimestampInMicroseconds updatePeriod, bool whiteBackground, std::string screenShotPath);

  // Destructor
  virtual ~Device();

  // Inits the screen
  void initScreen();

  // Getters and setters
  bool isCockpit() const {
    return cockpit;
  }

  Screen* getScreen() {
    return screen;
  }

  FontEngine *getFontEngine() {
    return fontEngine;
  }

  WidgetEngine *getWidgetEngine() {
    return widgetEngine;
  }

  GraphicEngine *getGraphicEngine() {
    return graphicEngine;
  }

  TimestampInMicroseconds getUpdatePeriod() const {
    return updatePeriod;
  }

  Int getNoChangeFrameCount() const {
    return noChangeFrameCount;
  }

  void setNoChangeFrameCount(Int noChangeFrameCount) {
    this->noChangeFrameCount = noChangeFrameCount;
  }

  const std::string& getName() const {
    return name;
  }

  GraphicObject *getVisibleWidgetPages() {
    return visibleWidgetPages;
  }

  void setVisibleWidgetPages(GraphicObject *visiblePages) {
    this->visibleWidgetPages = visiblePages;
  }

  TimestampInMicroseconds getNextUpdateTime() const {
    return nextUpdateTime;
  }

  void setNextUpdateTime(TimestampInMicroseconds nextUpdateTime) {
    this->nextUpdateTime = nextUpdateTime;
  }

  void setHeight(Int height) {
    this->height = height;
  }

  void setOrientation(GraphicScreenOrientation orientation) {
    this->orientation = orientation;
  }

  void setWidth(Int width) {
    this->width = width;
  }
};

} /* namespace GEODISCOVERER */

#endif /* DEVICE_H_ */
