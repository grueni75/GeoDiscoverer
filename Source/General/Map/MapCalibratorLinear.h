//============================================================================
// Name        : MapCalibratorLinear.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#ifndef MAPCALIBRATORLINEAR_H_
#define MAPCALIBRATORLINEAR_H_

namespace GEODISCOVERER {

class MapCalibratorLinear : public MapCalibrator {

protected:

  // Convert the geographic longitude / latitude coordinates to cartesian X / Y coordinates
  void convertGeographicToCartesian(MapPosition &pos);

  // Convert the cartesian X / Y coordinates to geographic longitude / latitude coordinates
  void convertCartesianToGeographic(MapPosition &pos);

public:

  // Constructors and destructor
  MapCalibratorLinear(bool doNotDelete=false);
  virtual ~MapCalibratorLinear();

};

} /* namespace GEODISCOVERER */
#endif /* MAPCALIBRATORLINEAR_H_ */
