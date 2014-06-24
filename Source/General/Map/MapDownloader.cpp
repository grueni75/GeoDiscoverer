//============================================================================
// Name        : MapDownloader.cpp
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

#include <Core.h>

namespace GEODISCOVERER {

// Map download thread
void *mapDownloaderThread(void *args) {
  ((MapDownloader*)args)->downloadMapImages();
  return NULL;
}

MapDownloader::MapDownloader(MapSourceMercatorTiles *mapSource) {
  downloadErrorWaitTime=core->getConfigStore()->getIntValue("Map","downloadErrorWaitTime");
  maxDownloadRetries=core->getConfigStore()->getIntValue("Map","maxDownloadRetries");
  maxMapArchiveSize=core->getConfigStore()->getIntValue("Map","maxMapArchiveSize");
  downloadQueueMutex=core->getThread()->createMutex("map downloader download queue mutex");
  downloadStartSignal=core->getThread()->createSignal();
  quitMapImageDownloadThread=false;
  this->mapSource=mapSource;
  mapImageDownloadThreadInfo=core->getThread()->createThread("map downloader thread",mapDownloaderThread,this);
}

MapDownloader::~MapDownloader() {
  deinit();
  // Wait until the download thread has finished
  //quitMapImageDownloadThread=true;
  //core->getThread()->issueSignal(downloadStartSignal);
  core->getThread()->cancelThread(mapImageDownloadThreadInfo);
  core->getThread()->waitForThread(mapImageDownloadThreadInfo);
  core->getThread()->destroyThread(mapImageDownloadThreadInfo);
  core->getThread()->destroyMutex(downloadQueueMutex);
  core->getThread()->destroySignal(downloadStartSignal);
}

// Deinitializes the map downloader
void MapDownloader::deinit() {
  // Delete all tile servers
  for(std::list<MapTileServer*>::iterator i=tileServers.begin();i!=tileServers.end();i++) {
    delete *i;
  }
  tileServers.clear();
}

// Initializes the map downloader
bool MapDownloader::init() {

  std::string mapPath=mapSource->getFolderPath();
  ZipArchive *mapArchive;

  // Merge all tiles from the previous session
  core->getScreen()->setWakeLock(true,false);
  std::list<ZipArchive*> *mapArchives=mapSource->lockMapArchives();
  mapArchive=mapArchives->front();
  std::string title="Merging tiles from last session with map " + mapSource->getFolder();
  DialogKey dialog=core->getDialog()->createProgress(title,0);
  struct dirent *dp;
  DIR *dfd;
  dfd=opendir(mapPath.c_str());
  if (dfd==NULL) {
    FATAL("can not read directory <%s>",mapPath.c_str());
    return false;
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
            mapArchive->addEntry(entryName,buffer,bufferSize);
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
  closedir(dfd);
  core->getDialog()->closeProgress(dialog);
  mapSource->unlockMapArchives();
  core->getScreen()->setWakeLock(core->getConfigStore()->getIntValue("General","wakeLock"),false);

  return true;
}

// Adds a map container to the download queue
void MapDownloader::queueMapContainerDownload(MapContainer *mapContainer)
{
  // Flag that the image of the map container is not yet there
  mapContainer->setDownloadComplete(false);

  // Request the download thread to fetch this image
  core->getThread()->lockMutex(downloadQueueMutex);
  downloadQueue.push_back(mapContainer);
  core->getThread()->unlockMutex(downloadQueueMutex);
  core->getThread()->issueSignal(downloadStartSignal);
}

// Adds a server url to the list of URLs a tile consists of
bool MapDownloader::addTileServer(std::string serverURL, double overlayAlpha) {
  MapTileServer *mapTileServer;

  // Check if the image is supported
  std::string imageFileExtension = serverURL.substr(serverURL.find_last_of(".")+1);
  ImageType imageType;
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

  // Remember the map tile server
  if (!(mapTileServer=new MapTileServer(mapSource,tileServers.size(),serverURL,overlayAlpha,imageType))) {
    FATAL("can not create map tile server object",NULL);
    return false;
  }
  tileServers.push_back(mapTileServer);
  return true;
}

// Downloads map tiles from the tile server
void MapDownloader::downloadMapImages() {

  std::list<std::string> status;

  // Set the priority
  core->getThread()->setThreadPriority(threadPriorityBackgroundLow);

  // This thread can be cancelled at any time
  core->getThread()->setThreadCancable();

  // Do an endless loop
  while (1) {

    // Wait for an update trigger
    core->getThread()->waitForSignal(downloadStartSignal);

    // Shall we quit?
    if (quitMapImageDownloadThread) {
      core->getThread()->exitThread();
    }

    // Loop until the queue is empty
    while (1) {

      // Get the next container from the queue
      MapContainer *mapContainer=NULL;
      Int totalLeft;
      core->getThread()->lockMutex(downloadQueueMutex);
      if (!downloadQueue.empty()) {

        // Prefer visible map containers
        totalLeft=downloadQueue.size();
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
      core->getThread()->unlockMutex(downloadQueueMutex);
      if (!mapContainer)
        break;

      // Change the status
      std::stringstream s;
      s << "Downloading " << totalLeft;
      if (totalLeft==1)
        s << " tile:";
      else
        s << " tiles:";
      status.push_back(s.str());
      status.push_back(mapContainer->getImageFileName());
      mapSource->setStatus(status);

      // Download all images
      bool downloadSuccess=true;
      UInt nr=0;
      for (std::list<MapTileServer*>::iterator i=tileServers.begin();i!=tileServers.end();i++) {
        MapTileServer *tileServer=*i;
        DownloadResult result = tileServer->downloadTileImage(mapContainer,nr);
        if (result == DownloadResultOtherFail) {
          downloadSuccess=false;
          break;
        }
        nr++;
      }

      // Process the image
      bool maxRetriesReached=(mapContainer->getDownloadRetries()>=maxDownloadRetries);
      if (downloadSuccess||maxRetriesReached) {
        if (downloadSuccess) {

          // Create one tile image out of the downloaded ones
          ImagePixel *composedImagePixel=NULL;
          Int composedImageWidth, composedImageHeight;
          std::string tempFilePath = mapSource->getFolderPath() + "/download.bin";
          remove(tempFilePath.c_str());
          for (std::list<MapTileServer*>::iterator i=tileServers.begin();i!=tileServers.end();i++) {
            MapTileServer *tileServer=*i;

            // Load the image and compose it with the existing image
            if (!tileServer->composeTileImage(composedImagePixel,composedImageWidth,composedImageHeight)) {
              if (composedImagePixel)
                free(composedImagePixel);
              composedImagePixel=NULL;
              break;
            }
          }
          if (composedImagePixel) {
            core->getImage()->writePNG(composedImagePixel,tempFilePath,composedImageWidth,composedImageHeight,core->getImage()->getRGBPixelSize());
            free(composedImagePixel);
          }

          // Add the image to the archive
          struct stat stat_buffer;
          bool tileAddedToArchive=false;
          if (stat(tempFilePath.c_str(),&stat_buffer)==0) {
            UByte *file_buffer = (UByte *)malloc(stat_buffer.st_size);
            if (file_buffer) {
              FILE *in;
              if ((in=fopen(tempFilePath.c_str(),"r"))) {
                fread(file_buffer,stat_buffer.st_size,1,in);
                fclose(in);
                std::list<ZipArchive*> *mapArchives=mapSource->lockMapArchives();
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
            mapContainer->writeCalibrationFile();
            std::list<ZipArchive*> *mapArchives=mapSource->lockMapArchives();
            mapArchives->back()->writeChanges();
            mapSource->unlockMapArchives();
          } else {
            WARNING("can not store <%s>",mapContainer->getImageFileName().c_str());
          }

        } else {
          WARNING("aborting download of <%s>",mapContainer->getImageFileName().c_str());
        }

        // Update the map cache
        mapSource->lockAccess();
        mapContainer->setDownloadComplete(true);
        mapSource->unlockAccess();
        core->getMapEngine()->setForceCacheUpdate();

      } else {

        // Put the container back in the queue if the retry count is not reached
        core->getThread()->lockMutex(downloadQueueMutex);
        downloadQueue.push_back(mapContainer);
        core->getThread()->unlockMutex(downloadQueueMutex);
        mapContainer->setDownloadRetries(mapContainer->getDownloadRetries()+1);

        // Wait some time before downloading again
        sleep(downloadErrorWaitTime);

      }

      // Change the status
      status.clear();
      mapSource->setStatus(status);

      // Shall we quit?
      if (quitMapImageDownloadThread) {
        core->getThread()->exitThread();
      }
    }
  }
}

} /* namespace GEODISCOVERER */
