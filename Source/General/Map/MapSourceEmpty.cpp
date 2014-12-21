//============================================================================
// Name        : MapSourceEmpty.cpp
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
