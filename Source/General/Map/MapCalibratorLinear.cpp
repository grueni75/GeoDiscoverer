//============================================================================
// Name        : MapCalibratorLinear.cpp
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

MapCalibratorLinear::MapCalibratorLinear(bool doNotDelete) : MapCalibrator(doNotDelete) {
  type=MapCalibratorTypeLinear;
}

MapCalibratorLinear::~MapCalibratorLinear() {
  // TODO Auto-generated destructor stub
}

// Convert the geographic longitude / latitude coordinates to cartesian X / Y coordinates
void MapCalibratorLinear::convertGeographicToCartesian(MapPosition &pos) {
  pos.setCartesianX(pos.getLng());
  pos.setCartesianY(pos.getLat());
}

// Convert the cartesian X / Y coordinates to geographic longitude / latitude coordinates
void MapCalibratorLinear::convertCartesianToGeographic(MapPosition &pos) {
  pos.setLng(pos.getCartesianX());
  pos.setLat(pos.getCartesianY());
}

} /* namespace GEODISCOVERER */
