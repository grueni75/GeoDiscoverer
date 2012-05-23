//============================================================================
// Name        : MapArea.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
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
MapArea::MapArea() {
  yNorth=std::numeric_limits<Int>::min();
  ySouth=std::numeric_limits<Int>::max();
  xEast=std::numeric_limits<Int>::max();
  xWest=std::numeric_limits<Int>::min();
  latNorth=-std::numeric_limits<double>::max();
  latSouth=+std::numeric_limits<double>::max();
  lngWest=+std::numeric_limits<double>::max();
  lngEast=-std::numeric_limits<double>::max();
  zoomLevel=0;
}

// Destructor
MapArea::~MapArea() {
}

// Operators
bool MapArea::operator==(const MapArea &rhs)
{
  if ((yNorth==rhs.getYNorth())&&(ySouth==rhs.getYSouth())&&
      (xWest==rhs.getXWest())&&(xEast==rhs.getXEast())&&
      (latNorth==rhs.getLatNorth())&&(latSouth==rhs.getLatSouth())&&
      (lngWest==rhs.getLngWest())&&(lngEast==rhs.getLngEast())&&
      (refPos==rhs.getRefPos()))
    return true;
  else
    return false;
}
bool MapArea::operator!=(const MapArea &rhs)
{
  return (!(*this==rhs));
}

// Getters and setters
MapPosition MapArea::getCenterPos() {
  MapPosition pos;
  pos.setLat(latSouth+(latNorth-latSouth)/2);
  pos.setLng(lngWest+(lngEast-lngWest)/2);
  return pos;
}

// Checks if the given position is contained in the area
bool MapArea::containsGeographicCoordinate(MapPosition pos) {
  if ((pos.getLng()>=lngWest)&&(pos.getLng()<=lngEast)&&(pos.getLat()>=latSouth)&&(pos.getLat()<=latNorth))
    return true;
  else
    return false;
}

}
