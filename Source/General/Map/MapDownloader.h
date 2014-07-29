//============================================================================
// Name        : MapDownloader.h
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

#ifndef MAPDOWNLOADER_H_
#define MAPDOWNLOADER_H_

namespace GEODISCOVERER {

class MapDownloader {

protected:

  MapSourceMercatorTiles *mapSource;                // Map source this object downloads for
  Int maxMapArchiveSize;                            // Maximum size a map archive shall have
  Int downloadErrorWaitTime;                        // Time in seconds to wait after a download error before starting a new download
  Int maxDownloadRetries;                           // Maximum number of retries before a download is aborted
  std::list<MapContainer*> downloadQueue;           // Queue of map containers that must be downloaded from the server
  ThreadMutexInfo *downloadQueueMutex;              // Mutex for accessing the download queue
  ThreadSignalInfo *downloadStartSignal;            // Signal that triggers the download thread
  bool quitMapImageDownloadThread;                  // Indicates that the map download image thread shall exit
  ThreadInfo *mapImageDownloadThreadInfo;           // Thread for downloading the map images
  std::list<MapTileServer*> tileServers;            // List of tile servers to download images from

public:

  // Constructor
  MapDownloader(MapSourceMercatorTiles *mapSource);

  // Destructor
  virtual ~MapDownloader();

  // Initialzes the downloader
  bool init();

  // Clears the downloader
  void deinit();

  // Adds a server url to the list of URLs a tile consists  of
  bool addTileServer(std::string serverURL, double overlayAlpha);

  // Adds a map container to the download queue
  void queueMapContainerDownload(MapContainer *mapContainer);

  // Downloads map tiles from the tile server
  void downloadMapImages();

  // Merges all so far downloaded zip archives into the first one
  void maintenance();
};

} /* namespace GEODISCOVERER */
#endif /* MAPDOWNLOADER_H_ */
