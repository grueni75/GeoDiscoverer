//============================================================================
// Name        : MapCalibratorSphericalNormalMercator.h
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

#include <MapCalibrator.h>

#ifndef MAPCALIBRATORSPHERICALNORMALMERCATOR_H_
#define MAPCALIBRATORSPHERICALNORMALMERCATOR_H_

namespace GEODISCOVERER {

class MapCalibratorSphericalNormalMercator : public MapCalibrator {

protected:

  // Convert the geographic longitude / latitude coordinates to cartesian X / Y coordinates
  void convertGeographicToCartesian(MapPosition &pos);

  // Convert the cartesian X / Y coordinates to geographic longitude / latitude coordinates
  void convertCartesianToGeographic(MapPosition &pos);

public:

  // Constructors and destructor
  MapCalibratorSphericalNormalMercator(bool doNotDelete=false);
  virtual ~MapCalibratorSphericalNormalMercator();

};

} /* namespace GEODISCOVERER */
#endif /* MAPCALIBRATORSPHERICALNORMALMERCATOR_H_ */
