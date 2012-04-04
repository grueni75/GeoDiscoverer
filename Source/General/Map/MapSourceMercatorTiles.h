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
  std::string tileServerURL;                        // URL of the tile server to use
  Int minZoomLevel;                                 // Minimum zoom value
  Int maxZoomLevel;                                 // Maximum zoom value
  Int mapContainerCacheSize;                        // Number of map containers to hold in the cache
  Int downloadErrorWaitTime;                        // Time in seconds to wait after a download error before starting a new download
  Int maxDownloadRetries;                           // Maximum number of retries before a download is aborted
  bool errorOccured;                                // Indicates that an error has occured
  bool downloadWarningOccured;                      // Indicates that a warning has occured
  static const double latBound;                     // Maximum allowed latitude value
  static const double lngBound;                     // Maximum allowed longitude value
  std::list<MapContainer*> downloadQueue;           // Queue of map containers that must be downloaded from the server
  ThreadMutexInfo *downloadQueueMutex;              // Mutex for accessing the download queue
  ThreadSignalInfo *downloadStartSignal;            // Signal that triggers the download thread
  bool quitMapImageDownloadThread;                  // Indicates that the map download image thread shall exit
  ThreadInfo *mapImageDownloadThreadInfo;           // Thread for downloading the map images

  // Replaces a variable in a string
  bool replaceVariableInTileServerURL(std::string &url, std::string variableName, std::string variableValue);

  // Creates a directory in the map folder
  bool createMapFolder(std::string path);

  // Downloads a map image from the server
  bool downloadMapImage(std::string url, std::string filePath);

  // Fetches the map tile in which the given position lies from disk or server
  MapTile *fetchMapTile(MapPosition pos, Int zoomLevel);

  // Returns the text contained in a xml node
  std::string getNodeText(XMLNode node);

  // Finds the best matching zoom level
  Int findBestMatchingZoomLevel(MapPosition pos);

  // Creates the calibrator for the given bounds
  MapCalibrator *createMapCalibrator(double latNorth, double latSouth, double lngWest, double lngEast);

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

  // Reads information about the map
  bool readGDSInfo();

  // Performs maintenance (e.g., recreate degraded search tree)
  virtual void maintenance();

  // Downloads map tiles from the tile server
  void downloadMapImages();

  // Returns the scale values for the given zoom level
  void getScales(Int zoomLevel, double &latScale, double &lngScale);

  // Finds the calibrator for the given position
  virtual MapCalibrator *findMapCalibrator(Int zoomLevel, MapPosition pos, bool &deleteCalibrator);

  // Returns the scale values for the given zoom level
  void getScales(Int zoomLevel, MapPosition pos, double &latScale, double &lngScale);

  // Getters and setters
  virtual void lockAccess() {
    core->getThread()->lockMutex(accessMutex);
  }

  virtual void unlockAccess() {
    core->getThread()->unlockMutex(accessMutex);
  }

};

} /* namespace GEODISCOVERER */

#endif /* MAPSOURCEMERCATORTILES_H_ */
