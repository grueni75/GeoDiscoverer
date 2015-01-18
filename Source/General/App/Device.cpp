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
  this->noChangeFrameCount=0;

  // Create components
  if (!(screen=new Screen(DPI,diagonal,updateInterval==0 ? false : true))) {
    FATAL("can not create screen object",NULL);
    return;
  }
  if (!(fontEngine=new FontEngine(screen->getDPI()))) {
    FATAL("can not create font engine object",NULL);
    return;
  }
  if (!(widgetEngine=new WidgetEngine(screen, fontEngine))) {
    FATAL("can not create widget engine object",NULL);
    return;
  }

}

// Destructor
Device::~Device() {

  // Delete components
  DEBUG("deleting widgetEngine",NULL);
  if (widgetEngine) delete widgetEngine;
  DEBUG("deleting fontEngine",NULL);
  if (fontEngine) delete fontEngine;
  DEBUG("deleting screen",NULL);
  if (screen) delete screen;

}

} /* namespace GEODISCOVERER */
