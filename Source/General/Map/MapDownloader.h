//============================================================================
// Name        : MapDownloader.h
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#ifndef MAPDOWNLOADER_H_
#define MAPDOWNLOADER_H_

namespace GEODISCOVERER {

class MapDownloader {

protected:

  MapSourceMercatorTiles *mapSource;                      // Map source this object downloads for
  Int maxMapArchiveSize;                                  // Maximum size a map archive shall have
  Int downloadErrorWaitTime;                              // Time in seconds to wait after a download error before starting a new download
  Int maxDownloadRetries;                                 // Maximum number of retries before a download is aborted
  std::list<MapContainer*> downloadQueue;                 // Queue of map containers that must be downloaded from the server
  ThreadMutexInfo *accessMutex;                           // Mutex for accessing the map downloader object
  std::vector<ThreadSignalInfo *> downloadStartSignals;   // Signals that triggers the download thread
  bool quitMapImageDownloadThread;                        // Indicates that the map download image thread shall exit
  std::vector<ThreadInfo *> mapImageDownloadThreadInfos;  // Threads for downloading the map images
  std::list<MapTileServer*> tileServers;                  // List of tile servers to download images from
  Int numberOfDownloadThreads;                            // Number of threads to spawn that download images
  std::vector<bool> downloadOngoing;                      // Indicates if the download thread is working

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
  void downloadMapImages(Int nr);

  // Merges all so far downloaded zip archives into the first one
  void maintenance();
};

} /* namespace GEODISCOVERER */
#endif /* MAPDOWNLOADER_H_ */
