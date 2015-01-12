//============================================================================
// Name        : Device.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#include <Core.h>

namespace GEODISCOVERER {

// Constructor
Device::Device(Int DPI, double diagonal, TimestampInMicroseconds updateInterval, bool cockpit) {

  // Update variables
  this->cockpit=cockpit;
  this->updateInterval=updateInterval;
  this->DPI=DPI;
  this->diagonal=diagonal;

  // Create components
  if (!(screen=new Screen(DPI,diagonal,updateInterval==0 ? false : true))) {
    FATAL("can not create screen object",NULL);
    return;
  }

}

// Destructor
Device::~Device() {
}

} /* namespace GEODISCOVERER */
