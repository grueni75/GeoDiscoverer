//============================================================================
// Name        : MapDownloader.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#include <Core.h>

namespace GEODISCOVERER {

// Map download thread
void *mapDownloaderThread(void *args) {
  UByte *argsBytes = (UByte *)args;
  MapDownloader *mapDownloader = *((MapDownloader**)&argsBytes[0]);
  Int threadNr = *((Int*)&argsBytes[sizeof(MapDownloader*)]);
  free(args);
  mapDownloader->downloadMapImages(threadNr);
  return NULL;
}

MapDownloader::MapDownloader(MapSourceMercatorTiles *mapSource) {
  downloadErrorWaitTime=core->getConfigStore()->getIntValue("Map","downloadErrorWaitTime",__FILE__, __LINE__);
  numberOfDownloadThreads=core->getConfigStore()->getIntValue("Map","numerOfDownloadThreads",__FILE__, __LINE__);
  maxDownloadRetries=core->getConfigStore()->getIntValue("Map","maxDownloadRetries",__FILE__, __LINE__);
  maxMapArchiveSize=core->getConfigStore()->getIntValue("Map","maxMapArchiveSize",__FILE__, __LINE__);
  accessMutex=core->getThread()->createMutex("map downloader access mutex");
  quitMapImageDownloadThread=false;
  this->mapSource=mapSource;
  downloadStartSignals.resize(numberOfDownloadThreads);
  mapImageDownloadThreadInfos.resize(numberOfDownloadThreads);
  downloadOngoing.resize(numberOfDownloadThreads);
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
    mapImageDownloadThreadInfos[i]=core->getThread()->createThread(threadName.str(),mapDownloaderThread,(void*)args);
    downloadOngoing[i]=false;
  }
}

MapDownloader::~MapDownloader() {
  deinit();
  for (Int i=0;i<numberOfDownloadThreads;i++) {
    core->getThread()->destroySignal(downloadStartSignals[i]);
  }
  core->getThread()->destroyMutex(accessMutex);
}

// Deinitializes the map downloader
void MapDownloader::deinit() {

  // Ensure that we have all mutexes
  core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);

  // Wait until the download thread has finished
  //quitMapImageDownloadThread=true;
  //core->getThread()->issueSignal(downloadStartSignal);
  for (Int i=0;i<numberOfDownloadThreads;i++) {
    if (mapImageDownloadThreadInfos[i]) {
      core->getThread()->cancelThread(mapImageDownloadThreadInfos[i]);
    }
  }
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

  // Request the download thread to fetch this image
  core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);
  downloadQueue.push_back(mapContainer);
  core->getThread()->unlockMutex(accessMutex);
  for (Int i=0;i<numberOfDownloadThreads;i++)
    core->getThread()->issueSignal(downloadStartSignals[i]);
}

// Adds a server url to the list of URLs a tile consists of
bool MapDownloader::addTileServer(std::string serverURL, double overlayAlpha, ImageType imageType, std::string layerGroupName, Int minZoomLevel, Int maxZoomLevel) {
  MapTileServer *mapTileServer;

  // Check if the image is supported
  if (imageType==ImageTypeUnknown) {
    std::string imageFileExtension = serverURL.substr(serverURL.find_last_of(".")+1);
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
  if (!(mapTileServer=new MapTileServer(mapSource,tileServers.size(),serverURL,overlayAlpha,imageType))) {
    FATAL("can not create map tile server object",NULL);
    return false;
  }
  tileServers.push_back(mapTileServer);
  return true;
}

// Downloads map tiles from the tile server
void MapDownloader::downloadMapImages(Int threadNr) {

  std::list<std::string> status;

  // Set the priority
  core->getThread()->setThreadPriority(threadPriorityBackgroundLow);

  // This thread can be cancelled at any time
  core->getThread()->setThreadCancable();

  // Do an endless loop
  while (1) {

    // Wait for an update trigger
    core->getThread()->waitForSignal(downloadStartSignals[threadNr]);

    // Shall we quit?
    if (quitMapImageDownloadThread) {
      core->getThread()->exitThread();
    }

    // Loop until the queue is empty
    while (1) {

      // Get the next container from the queue
      MapContainer *mapContainer=NULL;
      Int totalLeft=0;
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
        totalLeft=downloadQueue.size();
      }
      if (!mapContainer) {
        core->getThread()->unlockMutex(accessMutex);
        break;
      }

      // Change the status
      downloadOngoing[threadNr]=true;
      Int numerOfParallelDownloads=0;
      for (Int i=0;i<numberOfDownloadThreads;i++) {
        if (downloadOngoing[i])
          numerOfParallelDownloads++;
      }
      totalLeft+=numerOfParallelDownloads;
      std::stringstream s;
      s << "Downloading " << totalLeft;
      if (totalLeft==1)
        s << " tile:";
      else
        s << " tiles:";
      status.clear();
      status.push_back(s.str());
      s.str("");
      s << mapContainer->getImageFileName();
      if (numerOfParallelDownloads>1) {
        s << " (+" << numerOfParallelDownloads-1 << ")";
      }
      status.push_back(s.str());
      mapSource->setStatus(status,__FILE__, __LINE__);
      core->getThread()->unlockMutex(accessMutex);

      // Download all images
      bool downloadSuccess=true;
      bool oneTileFound=false;
      for (std::list<MapTileServer*>::iterator i=tileServers.begin();i!=tileServers.end();i++) {
        MapTileServer *tileServer=*i;
        DownloadResult result = tileServer->downloadTileImage(mapContainer,threadNr);
        if (result==DownloadResultOtherFail) {
          downloadSuccess=false;
          break;
        }
        if (result==DownloadResultSuccess)
          oneTileFound=true;
      }
      if (!oneTileFound)
        downloadSuccess=false;

      // Process the image
      bool maxRetriesReached=(mapContainer->getDownloadRetries()>=maxDownloadRetries);
      if (downloadSuccess||maxRetriesReached) {

        // Create one tile image out of the downloaded ones
        ImagePixel *composedImagePixel=NULL;
        Int composedImageWidth, composedImageHeight;
        std::stringstream tempFilePath;
        tempFilePath << mapSource->getFolderPath() << "/download." << threadNr << ".bin";
        remove(tempFilePath.str().c_str());
        for (std::list<MapTileServer*>::iterator i=tileServers.begin();i!=tileServers.end();i++) {
          MapTileServer *tileServer=*i;

          // Load the image and compose it with the existing image
          if (!tileServer->composeTileImage(composedImagePixel,composedImageWidth,composedImageHeight,threadNr)) {
            if (composedImagePixel)
              free(composedImagePixel);
            composedImagePixel=NULL;
            break;
          }
        }
        if (composedImagePixel) {
          core->getImage()->writePNG(composedImagePixel,tempFilePath.str(),composedImageWidth,composedImageHeight,core->getImage()->getRGBPixelSize());
          free(composedImagePixel);
        }

        // Add the image to the archive
        struct stat stat_buffer;
        bool tileAddedToArchive=false;
        Int result=stat(tempFilePath.str().c_str(),&stat_buffer);
        if (result==0) {
          UByte *file_buffer = (UByte *)malloc(stat_buffer.st_size);
          if (file_buffer) {
            FILE *in;
            if ((in=fopen(tempFilePath.str().c_str(),"r"))) {
              fread(file_buffer,stat_buffer.st_size,1,in);
              fclose(in);
              std::list<ZipArchive*> *mapArchives=mapSource->lockMapArchives(__FILE__, __LINE__);
              ZipArchive *mapArchive=mapArchives->back();
              if (mapArchive->getUnchangedSize()>maxMapArchiveSize) {
                std::stringstream newFilename;
                if (mapArchive->getArchiveName()=="tiles.gda")
                  newFilename << "tiles2.gda";
                else {
                  Int nr;
                  sscanf(mapArchive->getArchiveName().c_str(),"tiles%d.gda",&nr);
                  newFilename << "tiles" << nr+1 << ".gda";
                }
                mapArchive=new ZipArchive(mapArchive->getArchiveFolder(),newFilename.str());
                if ((mapArchive==NULL)||(!mapArchive->init()))
                  FATAL("can not create zip archive object",NULL);
                mapArchives->push_back(mapArchive);
              }
              mapArchive->addEntry(mapContainer->getImageFilePath(),file_buffer,stat_buffer.st_size);
              mapSource->unlockMapArchives();
              tileAddedToArchive=true;
            }
          }
        }

        // Write the gdm file
        if (tileAddedToArchive) {
          //DEBUG("writing calibration file of map container 0x%08x",mapContainer);
          mapContainer->writeCalibrationFile();
          std::list<ZipArchive*> *mapArchives=mapSource->lockMapArchives(__FILE__, __LINE__);
          mapArchives->back()->writeChanges();
          mapSource->unlockMapArchives();
        } else {
          WARNING("can not store <%s>",mapContainer->getImageFileName().c_str());
        }

        // Update the map cache
        mapSource->lockAccess(__FILE__, __LINE__);
        mapContainer->setDownloadComplete(true);
        mapSource->unlockAccess();
        core->getMapEngine()->setForceCacheUpdate(__FILE__, __LINE__);

      } else {

        // Put the container back in the queue if the retry count is not reached
        core->getThread()->lockMutex(accessMutex,__FILE__, __LINE__);
        downloadQueue.push_back(mapContainer);
        core->getThread()->unlockMutex(accessMutex);
        mapContainer->setDownloadRetries(mapContainer->getDownloadRetries()+1);

        // Wait some time before downloading again
        sleep(downloadErrorWaitTime);

      }

      // Change the status
      core->getThread()->lockMutex(accessMutex,__FILE__,__LINE__);
      downloadOngoing[threadNr]=false;
      numerOfParallelDownloads=0;
      for (Int i=0;i<numberOfDownloadThreads;i++) {
        if (downloadOngoing[i])
          numerOfParallelDownloads++;
      }
      if (numerOfParallelDownloads==0) {
        status.clear();
        mapSource->setStatus(status,__FILE__, __LINE__);
      }
      core->getThread()->unlockMutex(accessMutex);

      // Shall we quit?
      if (quitMapImageDownloadThread) {
        core->getThread()->exitThread();
      }
    }
  }
}

} /* namespace GEODISCOVERER */
