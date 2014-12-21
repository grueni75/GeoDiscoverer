//============================================================================
// Name        : MapCalibratorProj4.cpp
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

MapCalibratorProj4::MapCalibratorProj4(bool doNotDelete) : MapCalibrator(doNotDelete) {
  type=MapCalibratorTypeProj4;
  proj4State=NULL;
}

MapCalibratorProj4::~MapCalibratorProj4() {
}

// Inits the calibrator
void MapCalibratorProj4::init() {
  MapCalibrator::init();
  if (!(proj4State=pj_init_plus(args))) {
    ERROR("could not initialize map projection with arguments = \"%s\"", args);
  }
}

// Frees the calibrator
void MapCalibratorProj4::deinit() {
  pj_free(proj4State);
  MapCalibrator::deinit();
}


// Convert the geographic longitude / latitude coordinates to cartesian X / Y coordinates
void MapCalibratorProj4::convertGeographicToCartesian(MapPosition &pos) {
  projUV p;
  p.u=pos.getLngRad();
  p.v=pos.getLatRad();
  p = pj_fwd(p, proj4State);
  pos.setCartesianX(p.u);
  pos.setCartesianY(p.v);
}

// Convert the cartesian X / Y coordinates to geographic longitude / latitude coordinates
void MapCalibratorProj4::convertCartesianToGeographic(MapPosition &pos) {
  projUV p;
  p.u=pos.getCartesianX();
  p.v=pos.getCartesianY();
  p = pj_inv(p, proj4State);
  pos.setLng(FloatingPoint::rad2degree(p.u));
  pos.setLat(FloatingPoint::rad2degree(p.v));
}

} /* namespace GEODISCOVERER */
