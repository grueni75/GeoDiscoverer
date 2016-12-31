//============================================================================
// Name        : MapSourceEmpty.h
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
