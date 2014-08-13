//============================================================================
// Name        : MapSourceMercatorTiles.h
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

#ifndef MAPSOURCEMERCATORTILES_H_
#define MAPSOURCEMERCATORTILES_H_

namespace GEODISCOVERER {

class MapSourceMercatorTiles : public MapSource {

protected:

  ThreadMutexInfo *accessMutex;                     // Mutex for accessing the map source object
  Int minZoomLevel;                                 // Minimum zoom value
  Int maxZoomLevel;                                 // Maximum zoom value
  Int mapContainerCacheSize;                        // Number of map containers to hold in the cache
  bool errorOccured;                                // Indicates that an error has occured
  bool downloadWarningOccured;                      // Indicates that a warning has occured
  static const double latBound;                     // Maximum allowed latitude value
  static const double lngBound;                     // Maximum allowed longitude value
  MapDownloader *mapDownloader;                     // Downlads missing tiles from the tileserver

  // Fetches the map tile in which the given position lies from disk or server
  MapTile *fetchMapTile(MapPosition pos, Int zoomLevel);

  // Finds the best matching zoom level
  Int findBestMatchingZoomLevel(MapPosition pos);

  // Creates the calibrator for the given bounds
  MapCalibrator *createMapCalibrator(double latNorth, double latSouth, double lngWest, double lngEast);

  // Reads information about the map
  bool parseGDSInfo();

public:

  // Constructurs and destructor
  MapSourceMercatorTiles();
  virtual ~MapSourceMercatorTiles();

  // Initialzes the source
  virtual bool init();

  // Clears the source
  virtual void deinit();

  // Returns the map tile in which the position lies
  virtual MapTile *findMapTileByGeographicCoordinate(MapPosition pos, Int zoomLevel, bool lockZoomLevel, MapContainer *preferredMapContainer=NULL);

  // Returns the map tile that lies in a given area
  virtual MapTile *findMapTileByGeographicArea(MapArea area, MapTile *preferredNeigbor, MapContainer* &usedMapContainer);

  // Performs maintenance (e.g., recreate degraded search tree)
  virtual void maintenance();

  // Returns the scale values for the given zoom level
  virtual void getScales(Int zoomLevel, double &latScale, double &lngScale);

  // Finds the calibrator for the given position
  virtual MapCalibrator *findMapCalibrator(Int zoomLevel, MapPosition pos, bool &deleteCalibrator);

  // Getters and setters
  virtual void lockAccess(const char *file, int line) {
    core->getThread()->lockMutex(accessMutex, file, line);
  }

  virtual void unlockAccess() {
    core->getThread()->unlockMutex(accessMutex);
  }

  bool getDownloadWarningOccured() const {
    return downloadWarningOccured;
  }

  void setDownloadWarningOccured(bool downloadWarningOccured) {
    this->downloadWarningOccured = downloadWarningOccured;
  }

  bool getErrorOccured() const {
    return errorOccured;
  }

  void setErrorOccured(bool errorOccured) {
    this->errorOccured = errorOccured;
  }

  Int getMaxZoomLevel() const {
    return maxZoomLevel;
  }

  Int getMinZoomLevel() const {
    return minZoomLevel;
  }
};

} /* namespace GEODISCOVERER */

#endif /* MAPSOURCEMERCATORTILES_H_ */
