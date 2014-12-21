//============================================================================
// Name        : MapSourceEmpty.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#ifndef MAPSOURCEMPTY_H_
#define MAPSOURCEMPTY_H_

namespace GEODISCOVERER {

class MapSourceEmpty  : public MapSource {

public:

  // Constructurs and destructor
  MapSourceEmpty();
  virtual ~MapSourceEmpty();

  // Initialzes the source
  virtual bool init();

  // Returns the scale values for the given zoom level
  virtual void getScales(Int zoomLevel, double &latScale, double &lngScale);

  // Finds the calibrator for the given position
  virtual MapCalibrator *findMapCalibrator(Int zoomLevel, MapPosition pos, bool &deleteCalibrator);

};

}

#endif /* MAPSOURCEMPTY_H_ */
