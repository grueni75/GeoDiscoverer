//============================================================================
// Name        : MapArea.cpp
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
