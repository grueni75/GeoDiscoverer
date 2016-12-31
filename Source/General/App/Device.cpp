//============================================================================
// Name        : Device.cpp
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
  this->socketTimeout=core->getConfigStore()->getIntValue("Cockpit/App/Dashboard","socketTimeout",__FILE__,__LINE__);
  visibleWidgetPages=NULL;
  width=0;
  height=0;
  port=0;
  socketfd=-1;
  orientation=GraphicScreenOrientationProtrait;
  initDone=false;
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

// Creates the graphic
void Device::createGraphic() {

  // If screen is not initialized, skip this
  if (!screen)
    return;

  DEBUG("creating graphic",NULL);
  screen->setAllowAllocation(true);
  screen->createGraphic();
  fontEngine->createGraphic();
  widgetEngine->createGraphic();
  graphicEngine->createGraphic();
  screen->setAllowAllocation(false);
}

// Destroys the graphic
void Device::destroyGraphic(bool contextLost) {

  // If screen is not initialized, skip this
  if (!screen)
    return;

  // Do the destroying
  DEBUG("destroying graphic",NULL);
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
  if (!initDone)
    return false;

  // Do the drawing
  screen->setAllowAllocation(true);
  bool result=graphicEngine->draw(false);
  screen->setAllowAllocation(false);
  if (socketfd>=0)
    closeSocket();

  return result;
}

} /* namespace GEODISCOVERER */
