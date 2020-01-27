//============================================================================
// Name        : MapDownloader.cpp
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

#include <Core.h>

namespace GEODISCOVERER {

// Map download thread
void *mapDownloaderWorkerThread(void *args) {
  UByte *argsBytes = (UByte *)args;
  MapDownloader *mapDownloader = *((MapDownloader**)&argsBytes[0]);
  Int threadNr = *((Int*)&argsBytes[sizeof(MapDownloader*)]);
  free(args);
  mapDownloader->downloadMapImages(threadNr);
  return NULL;
}

// Map downloader statistics thread
void *mapDownloaderStatusThread(void *args) {
  MapDownloader *mapDownloader = (MapDownloader*)args;
  mapDownloader->updateDownloadStatus();
  return NULL;
}

// Map downloader image writer thread
void *mapDownloaderWriteImagesThread(void *args) {
  MapDownloader *mapDownloader = (MapDownloader*)args;
  mapDownloader->writeImages();
  return NULL;
}

MapDownloader::MapDownloader(MapSourceMercatorTiles *mapSource) {
  downloadErrorWaitTime=core->getConfigStore()->getIntValue("Map","downloadErrorWaitTime",__FILE__, __LINE__);
  numberOfDownloadThreads=core->getConfigStore()->getIntValue("Map","numerOfDownloadThreads",__FILE__, __LINE__);
  maxDownloadRetries=core->getConfigStore()->getIntValue("Map","maxDownloadRetries",__FILE__, __LINE__);
  downloadQueueRecommendedSize=core->getConfigStore()->getIntValue("Map","downloadQueueRecommendedSize",__FILE__, __LINE__);
  imageQueueMaxSize=core->getConfigStore()->getIntValue("Map","imageQueueMaxSize",__FILE__, __LINE__);
  downloadQueueRecommendedSizeExceeded=false;
  accessMutex=core->getThread()->createMutex("map downloader access mutex");
  quitThreads=false;
  this->mapSource=mapSource;
  downloadStartSignals.resize(numberOfDownloadThreads);
  mapImageDownloadThreadInfos.resize(numberOfDownloadThreads);
  downloadOngoing.resize(numberOfDownloadThreads);
  downloadStartTime=0;
  downloadedImages=0;
  for (Int i=0;i<numberOfDownloadThreads;i++) {
    downloadStartSignals[i]=core->getThread()->createSignal();
    UByte *args = (UByte *)malloc(sizeof(this)+sizeof(Int));
    if (!args) {
      FATAL("can not create args for map download thread",NULL);
      break;
    }
    *((MapDownloader**)&args[0])=this;
    *((Int*)&args[sizeof(this)])=i;
    std::stringstream threadName;
    threadName << "map downloader thread " << i;
    mapImageDownloadThreadInfos[i]=core->getThread()->createThread(threadName.str(),mapDownloaderWorkerThread,(void*)args);
    downloadOngoing[i]=false;
  }
  updateStatsStartSignal=core->getThread()->createSignal();
  updateStatsThreadInfo=core->getThread()->createThread("map downloader stats thread",mapDownloaderStatusThread,(void*)this);
  writeImagesStartSignal=core->getThread()->createSignal();
  writeImagesThreadInfo=core->getThread()->createThread("map downloader write image thread",mapDownloaderWriteImagesThread,(void*)this);
}

MapDownloader::~MapDownloader() {
  // deinit(); // Is now called by map source directly
  for (Int i=0;i<numberOfDownloadThreads;i++) {
    core->getThread()->destroySignal(downloadStartSignals[i]);
  }
  core->getThread()->destroySignal(updateStatsStartSignal);
  core->getThread()->destroyMutex(accessMutex);
  core->getThread()->destroySignal(writeImagesStartSignal);
}

// Deinitializes the map downloader
void MapDownloader::deinit() {

  // Stop all threads
  DEBUG("stopping update stats thread",NULL);
  quitThreads=true;
  core->getThread()->issueSignal(updateStatsStartSignal);
  core->getThread()->waitForThread(updateStatsThreadInfo);
  core->getThread()->destroyThread(updateStatsThreadInfo);
  updateStatsThreadInfo=NULL;
  DEBUG("stopping write images thread",NULL);
  core->getThread()->issueSignal(writeImagesStartSignal);
  core->getThread()->waitForThread(writeImagesThreadInfo);
  core->getThread()->destroyThread(writeImagesThreadInfo);
  writeImagesThreadInfo=NULL;

  // Ensure that we have all mutexes
  core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);

  // Wait until the download thread has finished
  DEBUG("cancelling map image download thread",NULL);
  for (Int i=0;i<numberOfDownloadThreads;i++) {
    if (mapImageDownloadThreadInfos[i]) {
      core->getThread()->cancelThread(mapImageDownloadThreadInfos[i]);
    }
  }
  DEBUG("stopping map image download thread",NULL);
  for (Int i=0;i<numberOfDownloadThreads;i++) {
    if (mapImageDownloadThreadInfos[i]) {
      core->getThread()->waitForThread(mapImageDownloadThreadInfos[i]);
      core->getThread()->destroyThread(mapImageDownloadThreadInfos[i]);
      mapImageDownloadThreadInfos[i]=NULL;
    }
  }

  // Unlock all mutexes
  core->getThread()->unlockMutex(accessMutex);

  // Delete all tile servers
  DEBUG("deleting tile servers",NULL);
  for(std::list<MapTileServer*>::iterator i=tileServers.begin();i!=tileServers.end();i++) {
    delete *i;
  }
  tileServers.clear();
}

// Initializes the map downloader
bool MapDownloader::init() {
  return true;
}

// Merges all so far downloaded zip archives into the first one
void MapDownloader::maintenance() {
  /*std::string mapPath=mapSource->getFolderPath();

  // Merge all tiles into a single archive
  struct dirent *dp;
  DIR *dfd;
  dfd=opendir(mapPath.c_str());
  if (dfd==NULL) {
    FATAL("can not read directory <%s>",mapPath.c_str());
    return;
  }
  while ((dp = readdir(dfd)) != NULL)
  {
    Int nr;

    // Copy files if this is a valid archive
    if (sscanf(dp->d_name,"tiles%d.gda",&nr)==1) {
      ZipArchive *archive = new ZipArchive(mapPath,dp->d_name);
      if ((archive)&&(archive->init())) {
        for (Int index=0;index<=archive->getEntryCount();index++) {
          std::string entryName = archive->getEntryFilename(index);
          ZipArchiveEntry entry = archive->openEntry(entryName);
          Int bufferSize = archive->getEntrySize(entryName);
          void *buffer = malloc(bufferSize);
          if ((entry)&&(buffer)) {
            archive->readEntry(entry,buffer,bufferSize);
            std::list<ZipArchive*> *mapArchives=mapSource->lockMapArchives();
            mapArchives->front()->addEntry(entryName,buffer,bufferSize);
            mapSource->unlockMapArchives();
            archive->closeEntry(entry);

          }
        }
        mapArchive->writeChanges();
        remove(std::string(mapPath + "/" + dp->d_name).c_str());
      }
    }

    // Remove any left over write tries
    std::string filename(dp->d_name);
    if ((filename.find(".zip.")!=std::string::npos)&&(filename.find("tiles")!=std::string::npos)) {
      remove((mapPath + "/" + filename).c_str());
    }
  }
  closedir(dfd);*/
}

// Adds a map container to the download queue
void MapDownloader::queueMapContainerDownload(MapContainer *mapContainer)
{
  // Flag that the image of the map container is not yet there
  mapContainer->setDownloadComplete(false);

  // Remember the start time
  core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);
  if ((downloadQueue.size()==0)&&(mapSource->countUnqueuedDownloadTiles()==0)) {
    downloadStartTime=core->getClock()->getSecondsSinceEpoch();
    downloadedImages=0;
  }

  // Request the download thread to fetch this image
  downloadQueue.push_back(mapContainer);
  if (downloadQueue.size()>downloadQueueRecommendedSize)
    downloadQueueRecommendedSizeExceeded=true;
  core->getThread()->unlockMutex(accessMutex);
  for (Int i=0;i<numberOfDownloadThreads;i++)
    core->getThread()->issueSignal(downloadStartSignals[i]);
}

// Clears the download queue
void MapDownloader::clearDownloadQueue()
{
  core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);
  if (!downloadQueue.empty()) {
    downloadQueue.clear();
  }
  core->getThread()->unlockMutex(accessMutex);
  core->getThread()->issueSignal(updateStatsStartSignal);
}

// Adds a server url to the list of URLs a tile consists of
bool MapDownloader::addTileServer(std::string serverURL, double overlayAlpha, ImageType imageType, std::string layerGroupName, Int minZoomLevel, Int maxZoomLevel, std::list<std::string> httpHeader) {
  MapTileServer *mapTileServer;

  // Check if the image is supported
  if (imageType==ImageTypeUnknown) {
    std::string imageFileExtension = serverURL.substr(serverURL.find_last_of(".")+1);
    Int endPos = imageFileExtension.find_first_of("?");
    if (endPos!=std::string::npos)
      imageFileExtension=imageFileExtension.substr(0,endPos);
    std::string imageFileExtensionLC=imageFileExtension;
    std::transform(imageFileExtensionLC.begin(),imageFileExtensionLC.end(),imageFileExtensionLC.begin(),::tolower);
    if (imageFileExtensionLC=="png") {
      imageType=ImageTypePNG;
    } else if (imageFileExtensionLC=="jpg") {
      imageType=ImageTypeJPEG;
    } else if (imageFileExtensionLC=="jpeg") {
      imageType=ImageTypeJPEG;
    } else {
      ERROR("unsupported image file extension <%s> in tile server url %s",imageFileExtension.c_str(),serverURL.c_str());
      mapSource->setErrorOccured(true);
      return false;
    }
  }

  // Remember the map tile server
  if (!(mapTileServer=new MapTileServer(mapSource,layerGroupName,tileServers.size(),serverURL,overlayAlpha,imageType,minZoomLevel,maxZoomLevel,httpHeader,numberOfDownloadThreads))) {
    FATAL("can not create map tile server object",NULL);
    return false;
  }
  tileServers.push_back(mapTileServer);

  // Remember the layer group name
  if (std::find(layerGroupNames.begin(), layerGroupNames.end(), layerGroupName)==layerGroupNames.end()) {
    layerGroupNames.push_back(layerGroupName);
  }

  return true;
}

// Defines the supported zoom levels
void MapDownloader::updateZoomLevels(Int &minZoomLevel,Int &maxZoomLevel, MapLayerNameMap &mapLayerNameMap) {

  // Set max and min zoom level
  // Harmonize zoom levels for tile servers
  minZoomLevel=0;
  Int minZoomLevelMap;
  Int maxZoomLevelMap=0;
  for (std::list<std::string>::iterator i=layerGroupNames.begin();i!=layerGroupNames.end();i++) {
    minZoomLevelMap=maxZoomLevelMap+1;
    maxZoomLevelMap=minZoomLevelMap;
    std::string layerGroupName=*i;
    Int minZoomLevelServer=std::numeric_limits<Int>::min();
    Int maxZoomLevelServer=std::numeric_limits<Int>::max();
    for (std::list<MapTileServer*>::iterator j=tileServers.begin();j!=tileServers.end();j++) {
      MapTileServer *server=*j;
      if (server->getLayerGroupName()==layerGroupName) {
        if (server->getMaxZoomLevelServer()<maxZoomLevelServer)
          maxZoomLevelServer=server->getMaxZoomLevelServer();
        if (server->getMinZoomLevelServer()>minZoomLevelServer)
          minZoomLevelServer=server->getMinZoomLevelServer();
      }
    }
    maxZoomLevelMap+=maxZoomLevelServer-minZoomLevelServer;
    for (std::list<MapTileServer*>::iterator j=tileServers.begin();j!=tileServers.end();j++) {
      MapTileServer *server=*j;
      if (server->getLayerGroupName()==layerGroupName) {
        server->setMaxZoomLevelServer(maxZoomLevelServer);
        server->setMinZoomLevelServer(minZoomLevelServer);
        server->setMaxZoomLevelMap(maxZoomLevelMap);
        server->setMinZoomLevelMap(minZoomLevelMap);
      }
    }
    Int k=minZoomLevelMap;
    for (Int j=minZoomLevelServer;j<=maxZoomLevelServer;j++) {
      std::stringstream name;
      if (layerGroupName=="")
        name << k;
      else
        name << layerGroupName << " " << j;
      mapLayerNameMap[name.str()]=k;
      //DEBUG("name=%s zoomLevel=%d",name.str().c_str(),k);
      k++;
    }
  }
  minZoomLevel=1;
  maxZoomLevel=maxZoomLevelMap;
}

// Returns the zoom level bounds of the map layer group that contains the refZoomLevel
void MapDownloader::getLayerGroupZoomLevelBounds(Int refZoomLevel, Int &minZoomLevelMap, Int &minZoomLevelServer, Int &maxZoomLevelServer) {
  minZoomLevelMap=-1;
  minZoomLevelServer=-1;
  maxZoomLevelServer=-1;
  for (std::list<MapTileServer*>::iterator i=tileServers.begin();i!=tileServers.end();i++) {
    MapTileServer *tileServer=*i;
    if ((refZoomLevel>=tileServer->getMinZoomLevelMap())&&(refZoomLevel<=tileServer->getMaxZoomLevelMap())) {
      minZoomLevelMap=tileServer->getMinZoomLevelMap();
      minZoomLevelServer=tileServer->getMinZoomLevelServer();
      maxZoomLevelServer=tileServer->getMaxZoomLevelServer();
      return;
    }
  }
}

// Downloads map tiles from the tile server
void MapDownloader::downloadMapImages(Int threadNr) {

  // Set the priority
  core->getThread()->setThreadPriority(threadPriorityBackgroundLow);

  // This thread can be cancelled at any time
  core->getThread()->setThreadCancable();

  // Do an endless loop
  while (1) {

    // Wait for an update trigger
    core->getThread()->waitForSignal(downloadStartSignals[threadNr]);

    // Shall we quit?
    if (quitThreads) {
      core->getThread()->exitThread();
    }

    // Loop until the queue is empty
    while (1) {

      // Get the next container from the queue
      MapContainer *mapContainer=NULL;
      core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);
      if (!downloadQueue.empty()) {

        // Prefer visible map containers
        for (std::list<MapContainer*>::iterator i=downloadQueue.begin();i!=downloadQueue.end();i++) {
          MapContainer *c=*i;
          if (c->isDrawn()) {
            mapContainer=c;
            downloadQueue.erase(i);
            break;
          }
        }
        if (!mapContainer) {
          mapContainer=downloadQueue.front();
          downloadQueue.pop_front();
        }
      }
      if (!mapContainer) {
        core->getThread()->unlockMutex(accessMutex);
        break;
      }

      // Change the status
      downloadOngoing[threadNr]=true;
      core->getThread()->unlockMutex(accessMutex);

      // Download all images
      bool downloadSuccess=true;
      bool oneTileFound=false;
      int fileNotFoundCount=0;
      std::vector<std::string> urls;
      for (std::list<MapTileServer*>::iterator i=tileServers.begin();i!=tileServers.end();i++) {
        MapTileServer *tileServer=*i;
        if ((mapContainer->getZoomLevelMap()>=tileServer->getMinZoomLevelMap())&&(mapContainer->getZoomLevelMap()<=tileServer->getMaxZoomLevelMap())) {
          std::string url;
          DownloadResult result = tileServer->downloadTileImage(mapContainer,threadNr,url);
          urls.push_back(url);
          if (result==DownloadResultFileNotFound) 
            fileNotFoundCount++;
          if (result==DownloadResultOtherFail) {
            DEBUG("other fail occured while downloading %s",url.c_str());
            downloadSuccess=false;
            break;
          }
          if (result==DownloadResultSuccess)
            oneTileFound=true;
        }
      }
      if (!oneTileFound)
        downloadSuccess=false;
      
      // Process the image
      bool maxRetriesReached=(mapContainer->getDownloadRetries()>=maxDownloadRetries);
      if (fileNotFoundCount==tileServers.size()) {
        DEBUG("no tile server has a tile at this position, skipping retry",NULL);
        maxRetriesReached=true;
      }
      if (downloadSuccess) {

        // Create one tile image out of the downloaded ones
        ImagePixel *composedImagePixel=NULL;
        Int composedImageWidth, composedImageHeight;
        Int j=0;
        for (std::list<MapTileServer*>::iterator i=tileServers.begin();i!=tileServers.end();i++) {
          MapTileServer *tileServer=*i;
          if ((mapContainer->getZoomLevelMap()>=tileServer->getMinZoomLevelMap())&&(mapContainer->getZoomLevelMap()<=tileServer->getMaxZoomLevelMap())) {

            // Load the image and compose it with the existing image
            if (!tileServer->composeTileImage(urls[j],composedImagePixel,composedImageWidth,composedImageHeight,threadNr)) {
              if (composedImagePixel)
                free(composedImagePixel);
              composedImagePixel=NULL;
              break;
            }
            j++;
          }
        }
        UByte *imageData=NULL;
        UInt imageSize;
        if (composedImagePixel) {
          imageData = core->getImage()->writePNG(composedImagePixel,composedImageWidth,composedImageHeight,core->getImage()->getRGBPixelSize(),imageSize);
          free(composedImagePixel);
        }

        // Queue the image
        if (imageData!=NULL) {
          MapImage image;
          image.imageData=imageData;
          image.imageSize=imageSize;
          image.mapContainer=mapContainer;
          Int size;
          do {
            core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
            size=imageQueue.size();
            if (size<imageQueueMaxSize) {
              imageQueue.push_back(image);
              core->getThread()->unlockMutex(accessMutex);
            } else {
              core->getThread()->unlockMutex(accessMutex);
              //DEBUG("image queue is full",NULL);
              sleep(1);
            }
          }
          while (size>=imageQueueMaxSize);
          core->getThread()->issueSignal(writeImagesStartSignal);
        }

      } else {

        // Put the container back in the queue if the retry count is not reached
        if (!maxRetriesReached) {
          mapContainer->setDownloadRetries(mapContainer->getDownloadRetries()+1);
          core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);
          downloadQueue.push_back(mapContainer);
          if (downloadQueue.size()>downloadQueueRecommendedSize)
            downloadQueueRecommendedSizeExceeded=true;
          core->getThread()->unlockMutex(accessMutex);

          // Wait some time before downloading again
          sleep(downloadErrorWaitTime);

        } else {

          // Mark the container as complete such that it can be removed
          //DEBUG("set download complete",NULL);
          mapSource->lockAccess(__FILE__, __LINE__);
          mapContainer->setDownloadComplete(true);
          mapContainer->setDownloadErrorOccured(true);
          mapSource->unlockAccess();
          core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);
          downloadedImages++;
          core->getThread()->unlockMutex(accessMutex);

        }

      }

      // Change the status
      core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
      downloadOngoing[threadNr]=false;
      core->getThread()->unlockMutex(accessMutex);
      core->getThread()->issueSignal(updateStatsStartSignal);

      // Shall we quit?
      if (quitThreads) {
        core->getThread()->exitThread();
      }
    }
  }
}

// Returns the number of active downloads
Int MapDownloader::countActiveDownloads() {
  core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
  Int numberOfParallelDownloads=0;
  for (Int i=0;i<numberOfDownloadThreads;i++) {
    if (downloadOngoing[i])
      numberOfParallelDownloads++;
  }
  core->getThread()->unlockMutex(accessMutex);
  return numberOfParallelDownloads;
}

// Indicates if the queue has exceeded its recommended size
bool MapDownloader::downloadQueueReachedRecommendedSize() {
  return downloadQueueRecommendedSizeExceeded;
}

// Updates the download status
void MapDownloader::updateDownloadStatus() {

  // Set the priority
  core->getThread()->setThreadPriority(threadPriorityBackgroundLow);

  // Do an endless loop
  while (1) {

    // Wait for trigger
    core->getThread()->waitForSignal(updateStatsStartSignal);

    // Shall we quit?
    if (quitThreads) {
      core->getThread()->exitThread();
    }

    // Queue empty?
    core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
    bool downloadQueueEmpty = (downloadQueue.size()==0) ? true : false;
    core->getThread()->unlockMutex(accessMutex);
    if (downloadQueueEmpty) {

      // If there are more tiles from a download job, trigger an update of the queue now
      if (mapSource->countUnqueuedDownloadTiles()>0) {
        //DEBUG("retriggering download job processing (%d tiles left)",mapSource->countUnqueuedDownloadTiles());
        mapSource->triggerDownloadJobProcessing();
      }
    }

    // Wait until all download jobs have been processed
    mapSource->lockDownloadJobProcessing(__FILE__,__LINE__);

    // Update the status
    core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
    Int numberOfParallelDownloads=0;
    Int imagesLeft=downloadQueue.size();
    if (imagesLeft<=downloadQueueRecommendedSize)
      downloadQueueRecommendedSizeExceeded=false;
    core->getThread()->unlockMutex(accessMutex);
    imagesLeft+=countActiveDownloads();
    imagesLeft+=mapSource->countUnqueuedDownloadTiles();
    TimestampInSeconds diff=core->getClock()->getSecondsSinceEpoch()-downloadStartTime;
    std::string timeLeft="unknown time";
    if (downloadedImages!=0) {
      double t=imagesLeft*((double)diff)/((double)downloadedImages);
      std::string value,unit;
      core->getUnitConverter()->formatTime(t,value,unit);
      timeLeft=value+" "+unit;
    }
    std::stringstream cmd;
    cmd << "updateMapDownloadStatus(";
    cmd << downloadedImages << "," << imagesLeft << "," << timeLeft << ")";
    //DEBUG("cmd=%s",cmd.str().c_str());
    core->getCommander()->dispatch(cmd.str());

    // Let the download job processing continue
    mapSource->unlockDownloadJobProcessing();

  }
}

// Updates the download status
void MapDownloader::writeImages() {

  // Set the priority
  core->getThread()->setThreadPriority(threadPriorityBackgroundHigh);

  // Do an endless loop
  while (1) {

    // Wait for trigger
    core->getThread()->waitForSignal(writeImagesStartSignal);

    // Shall we quit?
    if (quitThreads) {
      core->getThread()->exitThread();
    }

    while (1) {

      // Queue empty?
      //DEBUG("checking queue for new image",NULL);
      MapImage image;
      core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
      bool imageQueueEmpty = (imageQueue.size()==0) ? true : false;
      if (!imageQueueEmpty) {
        image=imageQueue.front();
        imageQueue.pop_front();
      }
      core->getThread()->unlockMutex(accessMutex);
      if ((imageQueueEmpty)||(quitThreads))
        break;

      // Add the image to the archive
      ZipArchive *mapArchive=NULL;
      //DEBUG("writing image data",NULL);
      mapArchive=new ZipArchive(image.mapContainer->getArchiveFileFolder(), image.mapContainer->getArchiveFileName());
      if ((mapArchive==NULL)||(!mapArchive->init()))
        FATAL("can not create zip archive object",NULL);
      mapArchive->addEntry(image.mapContainer->getImageFilePath(),(void*)image.imageData,(Int)image.imageSize);

      // Write the gdm file
      if (mapArchive) {
        //DEBUG("writing calibration data",NULL);
        image.mapContainer->writeCalibrationFile(mapArchive);
        mapArchive->writeChanges();
        delete mapArchive;
        mapArchive=NULL;
      } else {
        WARNING("can not store <%s>",image.mapContainer->getImageFileName().c_str());
      }

      // Update the map cache
      //DEBUG("set download complete",NULL);
      bool serveToRemoteMap;
      mapSource->lockAccess(__FILE__, __LINE__);
      image.mapContainer->setDownloadComplete(true);
      serveToRemoteMap=image.mapContainer->getServeToRemoteMap();
      std::string calibrationFilePath = image.mapContainer->getCalibrationFilePath();
      mapSource->unlockAccess();
      if (serveToRemoteMap) {
        std::stringstream cmd;
        cmd << "serveRemoteMapContainer(" << calibrationFilePath << ")";
        mapSource->queueRemoteServerCommand(cmd.str());
      }
      //DEBUG("set force cache update",NULL);
      core->getMapEngine()->setForceCacheUpdate(__FILE__, __LINE__);
      //DEBUG("inceasing download counter",NULL);
      core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);
      downloadedImages++;
      core->getThread()->unlockMutex(accessMutex);

    }

  }
}

} /* namespace GEODISCOVERER */
