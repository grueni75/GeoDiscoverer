//============================================================================
// Name        : MapSourceMercatorTiles.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#ifndef MAPSOURCEMERCATORTILES_H_
#define MAPSOURCEMERCATORTILES_H_

namespace GEODISCOVERER {

class MapSourceMercatorTiles : public MapSource {

protected:

  ThreadMutexInfo *accessMutex;                     // Mutex for accessing the map source object
  Int mapContainerCacheSize;                        // Number of map containers to hold in the cache
  bool errorOccured;                                // Indicates that an error has occured
  bool downloadWarningOccured;                      // Indicates that a warning has occured
  static const double latBound;                     // Maximum allowed latitude value
  static const double lngBound;                     // Maximum allowed longitude value
  std::list<MapContainer*> obsoleteMapContainers;   // List of obsolete map containers

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

  // Marks a map container as obsolete
  // Please note that other objects might still use this map container
  // Call unlinkMapContainer to solve this afterwards
  virtual void markMapContainerObsolete(MapContainer *c);

  // Removes all obsolete map containers
  virtual void removeObsoleteMapContainers(bool removeFromMapArchive);

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
