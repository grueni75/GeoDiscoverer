//============================================================================
// Name        : MapSourceEmpty.cpp
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

MapSourceEmpty::MapSourceEmpty()  : MapSource() {
  type=MapSourceTypeEmpty;
  isInitialized=true;
  centerPosition=new MapPosition();
  if (!centerPosition)
    FATAL("can not create map position object",NULL);
}

MapSourceEmpty::~MapSourceEmpty() {
  deinit();
  delete centerPosition;
}

// Initializes the source
bool MapSourceEmpty::init()
{
  return true;
}

// Returns the scale values for the given zoom level
void MapSourceEmpty::getScales(Int zoomLevel, double &latScale, double &lngScale) {
  lngScale=1;
  latScale=1;
}

// Finds the calibrator for the given position
MapCalibrator *MapSourceEmpty::findMapCalibrator(Int zoomLevel, MapPosition pos, bool &deleteCalibrator) {
  return NULL;
}

}