//============================================================================
// Name        : NavigationInfo.cpp
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

NavigationInfo::NavigationInfo() {
  type=NavigationInfoTypeUnknown;
  locationBearing=unknownAngle;
  targetBearing=unknownAngle;
  targetDistance=unknownDistance;
  targetDuration=unknownDuration;
  turnDistance=unknownDistance;
  turnAngle=unknownAngle;
  offRoute=false;
}

NavigationInfo::~NavigationInfo() {
  // TODO Auto-generated destructor stub
}

} /* namespace GEODISCOVERER */
