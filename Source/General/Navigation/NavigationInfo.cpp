//============================================================================
// Name        : NavigationInfo.cpp
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
