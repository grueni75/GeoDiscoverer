//============================================================================
// Name        : MapCalibratorProj.cpp
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
#include <MapCalibratorProj.h>
#include <MapPosition.h>

namespace GEODISCOVERER {

MapCalibratorProj::MapCalibratorProj(bool doNotDelete) : MapCalibrator(doNotDelete) {
  type=MapCalibratorTypeProj;
  projState=NULL;
}

MapCalibratorProj::~MapCalibratorProj() {
}

// Inits the calibrator
void MapCalibratorProj::init() {
  MapCalibrator::init();
  if (!(projState=proj_create(PJ_DEFAULT_CTX,args))) {
    ERROR("could not initialize map projection with arguments = \"%s\"", args);
  }
}

// Frees the calibrator
void MapCalibratorProj::deinit() {
  proj_destroy(projState);
  MapCalibrator::deinit();
}


// Convert the geographic longitude / latitude coordinates to cartesian X / Y coordinates
void MapCalibratorProj::convertGeographicToCartesian(MapPosition &pos) {
  PJ_COORD p;
  p.uv.u=pos.getLngRad();
  p.uv.v=pos.getLatRad();
  p = proj_trans(projState,PJ_FWD,p);
  //DEBUG("x=%f y=%f",p.uv.u,p.uv.v);
  pos.setCartesianX(p.uv.u);
  pos.setCartesianY(p.uv.v);
}

// Convert the cartesian X / Y coordinates to geographic longitude / latitude coordinates
void MapCalibratorProj::convertCartesianToGeographic(MapPosition &pos) {
  PJ_COORD p;
  p.uv.u=pos.getCartesianX();
  p.uv.v=pos.getCartesianY();
  p = proj_trans(projState,PJ_INV,p);
  //DEBUG("x=%f y=%f",p.uv.u,p.uv.v);
  pos.setLng(FloatingPoint::rad2degree(p.uv.u));
  pos.setLat(FloatingPoint::rad2degree(p.uv.v));
}

} /* namespace GEODISCOVERER */
