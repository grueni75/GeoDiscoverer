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

  TimestampInMicroseconds getUpdateInterval() const {
    return updateInterval;
  }
};

} /* namespace GEODISCOVERER */

#endif /* DEVICE_H_ */
