//============================================================================
// Name        : MapCalibratorMercator.h
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

#ifndef MAPCALIBRATORMERCATOR_H_
#define MAPCALIBRATORMERCATOR_H_

namespace GEODISCOVERER {

class MapCalibratorMercator : public MapCalibrator {

  // Computes the mercator projection for the given latitude
  double computeMercatorProjection(double value);

  // Computes the inverse mercator projection for the given value
  double computeInverseMercatorProjection(double value);

public:

  // Constructors and destructor
  MapCalibratorMercator(bool doNotDelete=false);
  virtual ~MapCalibratorMercator();

  // Updates the geographic coordinates (longitude and latitude) from the given picture coordinates
  virtual void setGeographicCoordinates(MapPosition &pos);

  // Updates the picture coordinates from the given geographic coordinates
  virtual bool setPictureCoordinates(MapPosition &pos);

};

} /* namespace GEODISCOVERER */
#endif /* MAPCALIBRATORMERCATOR_H_ */
