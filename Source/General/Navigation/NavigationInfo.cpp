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

const double NavigationInfo::unknownDistance = -1;
const double NavigationInfo::unknownDuration = -1;
const double NavigationInfo::unknownAngle = 999.0;
const double NavigationInfo::unknownSpeed = -1;

NavigationInfo::NavigationInfo() {
  type=NavigationInfoTypeUnknown;
  locationBearing=unknownAngle;
  locationSpeed=unknownSpeed;
  targetBearing=unknownAngle;
  targetDistance=unknownDistance;
  targetDuration=unknownDuration;
  turnDistance=unknownDistance;
  turnAngle=unknownAngle;
  routeDistance=unknownDistance;
  offRoute=false;
  trackLength=unknownDistance;
  altitude=unknownDistance;
}

bool NavigationInfo::operator==(const NavigationInfo &rhs)
{
  if ((type==rhs.getType()) &&
      (locationBearing==rhs.getLocationBearing()) &&
      (locationSpeed==rhs.getLocationSpeed()) &&
      (targetBearing==rhs.getTargetBearing()) &&
      (targetDistance==rhs.getTargetDistance()) &&
      (targetDuration==rhs.getTargetDuration()) &&
      (turnDistance==rhs.getTurnDistance()) &&
      (routeDistance==rhs.getRouteDistance()) &&
      (turnAngle==rhs.getTurnAngle()) &&
      (offRoute==rhs.getOffRoute()) &&
      (trackLength==rhs.getTrackLength()) &&
      (altitude==rhs.getAltitude())
  )
    return true;
  else
    return false;
}

bool NavigationInfo::operator!=(const NavigationInfo &rhs)
{
  return !(*this==rhs);
}

NavigationInfo::~NavigationInfo() {
  // TODO Auto-generated destructor stub
}

} /* namespace GEODISCOVERER */
