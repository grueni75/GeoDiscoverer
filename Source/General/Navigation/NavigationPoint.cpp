//============================================================================
// Name        : NavigationPoint.cpp
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

NavigationPoint::NavigationPoint() {
  MapPosition undefPos;
  lng = undefPos.getLng();
  lat = undefPos.getLat();
  timestamp=0;
  foreignTimestamp="0";
  distance=0;
  x=0;
  y=0;
  group="Default";
}

NavigationPoint::~NavigationPoint() {
  // TODO Auto-generated destructor stub
}

// Writes the point to the config
void NavigationPoint::writeToConfig(std::string path, TimestampInSeconds timestamp) {

  ConfigStore *configStore = core->getConfigStore();
  path=path + "[@name='" + getName() + "']";
  configStore->setStringValue(path,"address",address,__FILE__,__LINE__);
  configStore->setStringValue(path,"group",group,__FILE__,__LINE__);
  configStore->setDoubleValue(path,"lat",lat,__FILE__,__LINE__);
  configStore->setDoubleValue(path,"lng",lng,__FILE__,__LINE__);
  if (timestamp==0)
    timestamp=core->getClock()->getSecondsSinceEpoch();
  configStore->setLongValue(path,"timestamp",timestamp,__FILE__,__LINE__);
  configStore->setStringValue(path,"foreignTimestamp",foreignTimestamp,__FILE__,__LINE__);
}

// Reads the point from the config
bool NavigationPoint::readFromConfig(std::string path) {

  ConfigStore *configStore = core->getConfigStore();
  path=path + "[@name='" + getName() + "']";
  if (configStore->pathExists(path,__FILE__,__LINE__)) {
    address=configStore->getStringValue(path,"address",__FILE__,__LINE__);
    group=configStore->getStringValue(path,"group",__FILE__,__LINE__);
    lat=configStore->getDoubleValue(path,"lat",__FILE__,__LINE__);
    lng=configStore->getDoubleValue(path,"lng",__FILE__,__LINE__);
    timestamp=configStore->getLongValue(path,"timestamp",__FILE__,__LINE__);
    foreignTimestamp=configStore->getStringValue(path,"foreignTimestamp",__FILE__,__LINE__);
    return true;
  } else {
    return false;
  }
}

} /* namespace GEODISCOVERER */
