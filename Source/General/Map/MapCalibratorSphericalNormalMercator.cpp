//============================================================================
// Name        : MapCalibratorSphericalNormalMercator.cpp
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

MapCalibratorSphericalNormalMercator::MapCalibratorSphericalNormalMercator(bool doNotDelete) : MapCalibrator(doNotDelete) {
  type=MapCalibratorTypeSphericalNormalMercator;
}

MapCalibratorSphericalNormalMercator::~MapCalibratorSphericalNormalMercator() {
}

// Convert the geographic longitude / latitude coordinates to cartesian X / Y coordinates
void MapCalibratorSphericalNormalMercator::convertGeographicToCartesian(MapPosition &pos) {
  pos.setCartesianX(pos.getLng());
  pos.setCartesianY(log((sin(pos.getLatRad())+1)/cos(pos.getLatRad())));
}

// Convert the cartesian X / Y coordinates to geographic longitude / latitude coordinates
void MapCalibratorSphericalNormalMercator::convertCartesianToGeographic(MapPosition &pos) {
  pos.setLng(pos.getCartesianX());
  double t=pow(M_E,pos.getCartesianY());
  pos.setLat(FloatingPoint::rad2degree(2.0*atan((t-1)/(t+1))));
}

} /* namespace GEODISCOVERER */
