//============================================================================
// Name        : MapCalibratorSphericalNormalMercator.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

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
