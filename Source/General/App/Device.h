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

  // Density of the screen
  Int DPI;

  // Diagonal of the screen
  double diagonal;

  // Indicates if this device is rendering for a remote cockpit app
  bool cockpit;

  // Update interval
  TimestampInMicroseconds updateInterval;

  // Components of the device
  Screen *screen;
  FontEngine *fontEngine;
  WidgetEngine *widgetEngine;

  // Number of frames without any change
  Int noChangeFrameCount;

public:

  // Constructor
  Device(Int DPI, double diagonal, TimestampInMicroseconds updateInterval, bool cockpit);

  // Destructor
  virtual ~Device();

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

  TimestampInMicroseconds getUpdateInterval() const {
    return updateInterval;
  }

  Int getNoChangeFrameCount() const {
    return noChangeFrameCount;
  }

  void setNoChangeFrameCount(Int noChangeFrameCount) {
    this->noChangeFrameCount = noChangeFrameCount;
  }
};

} /* namespace GEODISCOVERER */

#endif /* DEVICE_H_ */
