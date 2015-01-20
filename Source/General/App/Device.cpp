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
Device::Device(std::string name, Int DPI, double diagonal, TimestampInMicroseconds updateInterval) {

  // Update variables
  this->name=name;
  if (name=="Default")
    this->cockpit=false;
  else
    this->cockpit=true;
  this->updateInterval=updateInterval;
  this->DPI=DPI;
  this->diagonal=diagonal;
  this->noChangeFrameCount=0;
  visibleWidgetPages=NULL;

  // Create components
  if (!(screen=new Screen(DPI,diagonal,updateInterval==0 ? false : true))) {
    FATAL("can not create screen object",NULL);
    return;
  }
  if (!(fontEngine=new FontEngine(screen->getDPI()))) {
    FATAL("can not create font engine object",NULL);
    return;
  }
  if (!(widgetEngine=new WidgetEngine(this))) {
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
