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
Device::Device(std::string name, bool whiteBackround, bool animationFriendly) {

  // Update variables
  this->name=name;
  this->whiteBackground=whiteBackround;
  this->animationFriendly=animationFriendly;
  this->noChangeFrameCount=0;
  this->DPI=0;
  this->diagonal=0;
  this->screen=NULL;
  this->fontEngine=NULL;
  this->widgetEngine=NULL;
  this->graphicEngine=NULL;
  visibleWidgetPages=NULL;
  width=0;
  height=0;
  port=0;
  socketfd=-1;
  orientation=GraphicScreenOrientationProtrait;
}

// Destructor
Device::~Device() {

  // Delete components
  //DEBUG("deleting widgetEngine of device %s",name.c_str());
  if (widgetEngine) delete widgetEngine;
  //DEBUG("deleting graphicEngine of device %s",name.c_str());
  if (graphicEngine) delete graphicEngine;
  //DEBUG("deleting fontEngine of device %s",name.c_str());
  if (fontEngine) delete fontEngine;
  //DEBUG("deleting screen of device %s",name.c_str());
  if (screen) delete screen;

  // Close socket
  closeSocket();
}

// Creates the components
void Device::init() {

  DEBUG("initializing screen of device %s",name.c_str());
  if (!(screen=new Screen(this))) {
    FATAL("can not create screen object",NULL);
    return;
  }
  DEBUG("initializing fontEngine of device %s",name.c_str());
  if (!(fontEngine=new FontEngine(screen))) {
    FATAL("can not create font engine",NULL);
    return;
  }
  DEBUG("initializing graphicEngine of device %s",name.c_str());
  if (!(graphicEngine=new GraphicEngine(this))) {
    FATAL("can not create graphic engine object",NULL);
    return;
  }
  DEBUG("initializing widgetEngine of device %s",name.c_str());
  if (!(widgetEngine=new WidgetEngine(this))) {
    FATAL("can not create widget engine object",NULL);
    return;
  }
}

// Reconfigures the device based on the new dimension
void Device::reconfigure() {
  screen->init(orientation,width,height);
  widgetEngine->updateWidgetPositions();
}

// Destroys the device
void Device::destroy(bool contextLost) {

  // If screen is not initialized, skip this
  if (!screen)
    return;

  // Do the destroying
  DEBUG("destroy",NULL);
  screen->setAllowDestroying(true);
  if (graphicEngine) graphicEngine->destroyGraphic();
  screen->destroyGraphic();
  if (widgetEngine) widgetEngine->destroyGraphic();
  if (fontEngine) fontEngine->destroyGraphic();
  screen->graphicInvalidated(contextLost);
  screen->setAllowDestroying(false);

}

// Performs the drawing on this device
bool Device::draw() {

  // Was the device info already discovered?
  if (width==0)
    return false;

  // Do the drawing
  getScreen()->setAllowAllocation(true);
  bool result=getGraphicEngine()->draw(false);
  getScreen()->setAllowAllocation(false);
  if (socketfd>=0)
    closeSocket();

  return result;
}
} /* namespace GEODISCOVERER */
