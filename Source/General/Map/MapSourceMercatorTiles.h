//============================================================================
// Name        : MapSourceMercatorTiles.h
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

#include <MapSource.h>
#include <MapArchiveFile.h>

#ifndef MAPSOURCEMERCATORTILES_H_
#define MAPSOURCEMERCATORTILES_H_

namespace GEODISCOVERER {

class MapSourceMercatorTiles : public MapSource {

protected:

  ThreadMutexInfo *accessMutex;                     // Mutex for accessing the map source object
  Int mapContainerCacheSize;                        // Number of map containers to hold in the cache
  Int downloadAreaLength;                           // Length of the square in kilometers that is downloaded around a route position
  Int downloadAreaMinDistance;                      // Distance in kilometers that a route point must be away from the previous one before a new area is downloaded.
  bool errorOccured;                                // Indicates that an error has occured
  bool downloadWarningOccured;                      // Indicates that a warning has occured
  static const double latBound;                     // Maximum allowed latitude value
  static const double lngBound;                     // Maximum allowed longitude value
  std::list<MapContainer*> obsoleteMapContainers;   // List of obsolete map containers
  ThreadMutexInfo *downloadJobsMutex;               // Mutex for accessing infos related to download jobs
  //std::list<std::string> processedDownloadJobs;     // List of job names that have been processed already
  ThreadMutexInfo *processDownloadJobsThreadMutex;  // Mutex for accessing the process download jobs thread
  ThreadInfo *processDownloadJobsThreadInfo;        // Thread that processes all download jobs
  bool quitProcessDownloadJobsThread;               // Indicates if the process download jobs thread shall quit
  Int unqueuedDownloadTileCount;                    // Number of tiles that are not yet queued but must be downloaded
  TimestampInSeconds lastGDSModification;           // Time when the GDS for this map was last modified
  TimestampInSeconds lastGDMModification;           // Time when the newest GDM for this map was last modified
  Long mapFolderDiskUsage;                          // Current size of the map folder in bytes
  Long mapFolderMaxSize;                            // Maximum size of the map folder in Bytes to maintain

  // Fetches the map tile in which the given position lies from disk or server
  MapTile *fetchMapTile(MapPosition pos, Int zoomLevel);

  // Finds the best matching zoom level
  Int findBestMatchingZoomLevel(MapPosition pos, Int refZoomLevelMap, Int &minZoomLevelMap, Int &minZoomLevelServer);

  // Creates the calibrator for the given bounds
  MapCalibrator *createMapCalibrator(double latNorth, double latSouth, double lngWest, double lngEast);

  // Reads information about the map
  bool parseGDSInfo();

  // Clears the given map directory
  void cleanMapFolder(std::string dirPath,MapArea *displayArea,bool allZoomLevels, bool removeAll=false, bool updateFileList=false);

  // Computes the mercator x and y ranges for the given area and zoom level
  void computeMercatorBounds(MapArea *displayArea,Int zMap,Int &zServer,Int &startX,Int &endX,Int &startY,Int &endY);

  // Starts the download job processing
  void startDownloadJobProcessing();

  // Creates the file path for the given tile
  void createTilePath(Int zMap, Int x, Int y, std::stringstream &archiveFileFolder, std::stringstream &archiveFileBase);

public:

  // Constructurs and destructor
  MapSourceMercatorTiles(TimestampInSeconds lastGDSModification);
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
  virtual void removeObsoleteMapContainers(MapArea *displayArea=NULL, bool allZoomLevels=false);

  // Adds a download job from the current visible map
  virtual void addDownloadJob(bool estimateOnly, std::string routeName, std::string zoomLevels);

  // Processes all pending download jobs
  virtual void processDownloadJobs();

  // Indicates if the map source has download jobs
  virtual bool hasDownloadJobs();

  // Stops the download job processing
  virtual void stopDownloadJobProcessing();

  // Returns the number of unqueued tiles
  virtual Int countUnqueuedDownloadTiles();

  // Trigger the download job processing
  virtual void triggerDownloadJobProcessing();

  // Removes all download jobs
  virtual void clearDownloadJobs();

  // Ensures that all threads that download tiles are stopped
  virtual void stopDownloadThreads();

  // Returns the map tile for the given mercator coordinates
  MapTile *fetchMapTile(Int z, Int x, Int y);

  // Returns the zoom level of the server
  virtual Int getServerZoomLevel(Int mapZoomLevel);

  // Fetches a map tile and returns its image data
  virtual UByte *fetchMapTile(Int z, Int x, Int y, double saturationOffset, double brightnessOffset, UInt &imageSize);

  // Inserts the new map archive file into the file list
  virtual void updateMapArchiveFiles(std::string filePath);

  // Getters and setters
  virtual void lockAccess(const char *file, int line) {
    core->getThread()->lockMutex(accessMutex, file, line);
  }

  virtual void unlockAccess() {
    core->getThread()->unlockMutex(accessMutex);
  }

  virtual void lockDownloadJobProcessing(const char *file, int line);

  virtual void unlockDownloadJobProcessing();

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
