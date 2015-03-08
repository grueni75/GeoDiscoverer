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
Device::Device(std::string name, Int DPI, double diagonal, TimestampInMicroseconds updatePeriod, bool whiteBackround, std::string screenShotPath) {

  // Update variables
  this->name=name;
  if (name=="Default")
    this->cockpit=false;
  else
    this->cockpit=true;
  this->updatePeriod=updatePeriod;
  this->DPI=DPI;
  this->diagonal=diagonal;
  this->noChangeFrameCount=0;
  visibleWidgetPages=NULL;
  nextUpdateTime=0;

  // Create components
  DEBUG("initializing screen of device %s",name.c_str());
  if (!(screen=new Screen(DPI,diagonal,updatePeriod==0 ? false : true, whiteBackround, screenShotPath))) {
    FATAL("can not create screen object",NULL);
    return;
  }
  DEBUG("initializing fontEngine of device %s",name.c_str());
  if (!(fontEngine=new FontEngine(screen->getDPI()))) {
    FATAL("can not create font engine",NULL);
    return;
  }
  DEBUG("initializing widgetEngine of device %s",name.c_str());
  if (!(widgetEngine=new WidgetEngine(this))) {
    FATAL("can not create widget engine object",NULL);
    return;
  }
  DEBUG("initializing graphicEngine of device %s",name.c_str());
  if (!(graphicEngine=new GraphicEngine(this))) {
    FATAL("can not create graphic engine object",NULL);
    return;
  }

}

// Destructor
Device::~Device() {

  // Delete components
  DEBUG("deleting graphicEngine of device %s",name.c_str());
  if (graphicEngine) delete graphicEngine;
  DEBUG("deleting widgetEngine of device %s",name.c_str());
  if (widgetEngine) delete widgetEngine;
  DEBUG("deleting fontEngine of device %s",name.c_str());
  if (fontEngine) delete fontEngine;
  DEBUG("deleting screen of device %s",name.c_str());
  if (screen) delete screen;

}

// Inits the screen
void Device::initScreen() {
  screen->init(orientation,width,height);
}

} /* namespace GEODISCOVERER */
