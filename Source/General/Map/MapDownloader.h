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

typedef std::map<std::string, Int> MapLayerNameMap;
typedef std::pair<std::string, Int> MapLayerNamePair;

struct MapImage {
  UByte *imageData;
  UInt imageSize;
  MapContainer *mapContainer;
};

class MapDownloader {

protected:

  MapSourceMercatorTiles *mapSource;                      // Map source this object downloads for
  Int downloadErrorWaitTime;                              // Time in seconds to wait after a download error before starting a new download
  Int maxDownloadRetries;                                 // Maximum number of retries before a download is aborted
  std::list<MapContainer*> downloadQueue;                 // Queue of map containers that must be downloaded from the server
  ThreadMutexInfo *accessMutex;                           // Mutex for accessing the map downloader object
  std::vector<ThreadSignalInfo *> downloadStartSignals;   // Signals that triggers the download thread
  bool quitThreads;                                       // Indicates that the map download image and the status thread shall exit
  std::vector<ThreadInfo *> mapImageDownloadThreadInfos;  // Threads for downloading the map images
  ThreadSignalInfo *updateStatsStartSignal;               // Signal for starting the status update computation
  ThreadInfo *updateStatsThreadInfo;                      // Thread that computes statistics about the download
  std::list<MapTileServer*> tileServers;                  // List of tile servers to download images from
  Int numberOfDownloadThreads;                            // Number of threads to spawn that download images
  std::vector<bool> downloadOngoing;                      // Indicates if the download thread is working
  std::list<std::string> layerGroupNames;                 // List of used layer group names
  TimestampInSeconds downloadStartTime;                   // Last time the first download was started
  Int downloadedImages;                                   // Number of downloaded images so far
  Int downloadQueueRecommendedSize;                       // Size that the download queue should not exceed
  bool downloadQueueRecommendedSizeExceeded;              // Indicates that the size of the download queue exceeds the recommended size
  std::list<MapImage> imageQueue;                         // Queue of images that must be written to storage
  UInt imageQueueMaxSize;                                 // Maximum size of the image queue
  ThreadSignalInfo *writeImagesStartSignal;               // Signal for starting the writing of images to storage
  ThreadInfo *writeImagesThreadInfo;                      // Thread that writes images to storage

public:

  // Constructor
  MapDownloader(MapSourceMercatorTiles *mapSource);

  // Destructor
  virtual ~MapDownloader();

  // Initialzes the downloader
  bool init();

  // Clears the downloader
  void deinit();

  // Downloads map tiles from the tile server
  void downloadMapImages(Int nr);

  // Updates the download status
  void updateDownloadStatus();

  // Adds a server url to the list of URLs a tile consists  of
  bool addTileServer(std::string serverURL, double overlayAlpha, ImageType imageType, std::string layerGroupName, Int minZoomLevel, Int maxZoomLevel);

  // Adds a map container to the download queue
  void queueMapContainerDownload(MapContainer *mapContainer);

  // Merges all so far downloaded zip archives into the first one
  void maintenance();

  // Defines the supported zoom levels
  void updateZoomLevels(Int &minZoomLevel,Int &maxZoomLevel, MapLayerNameMap &mapLayerNameMap);

  // Returns the zoom level bounds of the map layer group that contains the refZoomLevel
  void getLayerGroupZoomLevelBounds(Int refZoomLevel, Int &minZoomLevelMap, Int &minZoomLevelServer, Int &maxZoomLevelServer);

  // Returns the number of active downloads
  Int countActiveDownloads();

  // Indicates if the queue has exceeded its recommended size
  bool downloadQueueReachedRecommendedSize();

  // Writes downloaded images to storage
  void writeImages();

};

} /* namespace GEODISCOVERER */
#endif /* MAPDOWNLOADER_H_ */
