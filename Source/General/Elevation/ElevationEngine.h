//============================================================================
// Name        : ElevationEngine.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010-2021 Matthias Gruenewald
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

#include <MapArea.h>
#include <ElevationEnginePlatform.h>

#ifndef ELEVATIONENGINE_H_
#define ELEVATIONENGINE_H_

namespace GEODISCOVERER {

class ElevationEngine {

protected:

  bool isInitialized;                             // Indicates if the object is initialized
  std::string demFolderPath;                      // Path to the DEM folder
  ThreadMutexInfo *accessMutex;                   // Mutex controlling the access to this object
  const Int lowResZoomLevel=9;                    // Zoom level from which to use the low resolution dem data
  Int workerCount;                                // Number of workers to use for rendering hillshades
  DEMDataset **demDatasetFullRes;                 // Dataset to use (full resolution version)
  DEMDataset **demDatasetLowRes;                  // Dataset to use (low resoultion version)
  bool *demDatasetBusy;                           // Indicates if the given dataset is in use
  ThreadSignalInfo *demDatasetReadySignal;        // Indicates that one of the datasets is not busy anymore

  // Converts a tile y number into latitude
  double tiley2lat(Int y, Int worldRes);

  // Converts a tile x number into longitude
  double tilex2lon(Int x, Int worldRes);

  // Resets dem dataset busy indicator for the given worker number
  void resetDemDatasetBusy(Int workerNr);

public:

  // Constructurs and destructor
  ElevationEngine();
  ~ElevationEngine();

  // Initialzes the engine
  bool init();

  // Clears the engine
  void deinit();

  // Creates a hillshading for the given map area
  bool renderHillshadeTile(Int z, Int y, Int x, std::string imageFilename);

  // Getters and setters
  bool getIsInitialized() const
  {
    return isInitialized;
  }

};

}

#endif /* MAPSOURCE_H_ */
