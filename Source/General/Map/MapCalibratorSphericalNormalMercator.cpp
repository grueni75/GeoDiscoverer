//============================================================================
// Name        : MapCalibratorSphericalNormalMercator.cpp
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
