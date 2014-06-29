//============================================================================
// Name        : MapCalibratorProj4.h
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
