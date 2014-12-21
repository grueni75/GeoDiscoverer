//============================================================================
// Name        : MapCalibratorProj4.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#ifndef MAPCALIBRATORPROJ4_H_
#define MAPCALIBRATORPROJ4_H_

#include <proj_api.h>

namespace GEODISCOVERER {

class MapCalibratorProj4 : public MapCalibrator {

protected:

  projPJ proj4State; // Pointer to the proj4 state

  // Convert the geographic longitude / latitude coordinates to cartesian X / Y coordinates
  void convertGeographicToCartesian(MapPosition &pos);

  // Convert the cartesian X / Y coordinates to geographic longitude / latitude coordinates
  void convertCartesianToGeographic(MapPosition &pos);

public:

  // Constructors and destructor
  MapCalibratorProj4(bool doNotDelete=false);
  virtual ~MapCalibratorProj4();

  // Inits the calibrator
  void init();

  // Frees the calibrator
  void deinit();

};

} /* namespace GEODISCOVERER */
#endif /* MAPCALIBRATORPROJ4_H_ */
