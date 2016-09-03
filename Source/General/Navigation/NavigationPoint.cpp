//============================================================================
// Name        : NavigationPoint.cpp
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

NavigationPoint::NavigationPoint() {
  MapPosition undefPos;
  lng = undefPos.getLng();
  lat = undefPos.getLat();
  graphicPrimitiveKey=0;
  timestamp=0;
}

NavigationPoint::~NavigationPoint() {
  // TODO Auto-generated destructor stub
}

// Writes the point to the config
void NavigationPoint::writeToConfig(std::string path, TimestampInSeconds timestamp) {

  ConfigStore *configStore = core->getConfigStore();
  path=path + "[@name='" + getName() + "']";
  configStore->setStringValue(path,"address",address,__FILE__,__LINE__);
  configStore->setDoubleValue(path,"lat",lat,__FILE__,__LINE__);
  configStore->setDoubleValue(path,"lng",lng,__FILE__,__LINE__);
  if (timestamp==0)
    timestamp=core->getClock()->getSecondsSinceEpoch();
  configStore->setLongValue(path,"timestamp",timestamp,__FILE__,__LINE__);
}

// Reads the point from the config
bool NavigationPoint::readFromConfig(std::string path) {

  ConfigStore *configStore = core->getConfigStore();
  path=path + "[@name='" + getName() + "']";
  if (configStore->pathExists(path,__FILE__,__LINE__)) {
    address=configStore->getStringValue(path,"address",__FILE__,__LINE__);
    lat=configStore->getDoubleValue(path,"lat",__FILE__,__LINE__);
    lng=configStore->getDoubleValue(path,"lng",__FILE__,__LINE__);
    timestamp=configStore->getLongValue(path,"timestamp",__FILE__,__LINE__);
    return true;
  } else {
    return false;
  }
}

} /* namespace GEODISCOVERER */
