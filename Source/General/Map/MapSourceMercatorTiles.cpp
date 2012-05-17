//============================================================================
// Name        : MapSourceMercatorTiles.cpp
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

// Constant values
const double MapSourceMercatorTiles::latBound = 85.0511287798;
const double MapSourceMercatorTiles::lngBound = 180.0;

// Map download thread
void *mapSourceMercatorTilesDownloadThread(void *args) {
  ((MapSourceMercatorTiles*)args)->downloadMapImages();
  return NULL;
}

// Constructor
MapSourceMercatorTiles::MapSourceMercatorTiles() : MapSource() {
  type=MapSourceTypeMercatorTiles;
  mapContainerCacheSize=core->getConfigStore()->getIntValue("Map","mapContainerCacheSize");
  downloadErrorWaitTime=core->getConfigStore()->getIntValue("Map","downloadErrorWaitTime");
  maxDownloadRetries=core->getConfigStore()->getIntValue("Map","maxDownloadRetries");
  mapTileLength=256;
  accessMutex=core->getThread()->createMutex();
  errorOccured=false;
  downloadWarningOccured=false;
  downloadQueueMutex=core->getThread()->createMutex();
  downloadStartSignal=core->getThread()->createSignal();
  quitMapImageDownloadThread=false;
  mapImageDownloadThreadInfo=core->getThread()->createThread(mapSourceMercatorTilesDownloadThread,this);
}

// Destructor
MapSourceMercatorTiles::~MapSourceMercatorTiles() {

  // Wait until the download thread has finished
  quitMapImageDownloadThread=true;
  core->getThread()->issueSignal(downloadStartSignal);
  core->getThread()->waitForThread(mapImageDownloadThreadInfo);
  core->getThread()->destroyThread(mapImageDownloadThreadInfo);

  // Free everything else
  deinit();
  core->getThread()->destroyMutex(accessMutex);
  core->getThread()->destroyMutex(downloadQueueMutex);
  core->getThread()->destroySignal(downloadStartSignal);
}

// Deinitializes the map source
void MapSourceMercatorTiles::deinit() {
  MapSource::deinit();
}

// Initializes the map source
bool MapSourceMercatorTiles::init() {

  std::string mapPath=getFolderPath();

  // Read the information from the config file
  if (!readGDSInfo())
    return false;

  // Init the center position
  centerPosition.setLat(0);
  centerPosition.setLng(0);
  centerPosition.setLatScale(((double)mapTileLength*pow(2.0,(double)minZoomLevel))/2.0/MapSourceMercatorTiles::latBound);
  centerPosition.setLngScale(((double)mapTileLength*pow(2.0,(double)minZoomLevel))/2.0/MapSourceMercatorTiles::lngBound);

  // Init the zoom levels
  for (int z=0;z<(maxZoomLevel-minZoomLevel)+2;z++) {
    zoomLevelSearchTrees.push_back(NULL);
  }

  // Finished
  isInitialized=true;
  return true;
}

// Replaces a variable in a string
bool MapSourceMercatorTiles::replaceVariableInTileServerURL(std::string &url, std::string variableName, std::string variableValue) {
  size_t pos;
  pos=url.find(variableName);
  if (pos==std::string::npos) {
    ERROR("variable %s not found in tile server URL <%s>",variableName.c_str(),tileServerURL.c_str());
    errorOccured=true;
    return false;
  }
  url.replace(pos,variableName.size(),variableValue);
  return true;
}

// Creates a directory in the map folder
bool MapSourceMercatorTiles::createMapFolder(std::string path) {
  if (access(path.c_str(), F_OK) != 0)
  {
    if (mkdir(path.c_str(),S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
      if (!errorOccured) {
        ERROR("can not create map directory <%s>",path.c_str());
        errorOccured=true;
      }
      return false;
    }
  }
  return true;
}

// Finds the best matching zoom level
Int MapSourceMercatorTiles::findBestMatchingZoomLevel(MapPosition pos) {
  Int z=minZoomLevel;
  double latSouth, latNorth, lngEast, lngWest;
  double distToLngScale,distToLatScale;
  double distToNearestLngScale=-1,distToNearestLatScale;
  double lngScale,latScale;
  bool newCandidateFound;
  for (int i=minZoomLevel;i<=maxZoomLevel;i++) {
    pos.computeMercatorTileBounds(i,latNorth,latSouth,lngWest,lngEast);
    lngScale=mapTileLength/fabs(lngEast-lngWest);
    latScale=mapTileLength/fabs(latNorth-latSouth);
    distToLngScale=fabs(lngScale-pos.getLngScale());
    distToLatScale=fabs(latScale-pos.getLatScale());
    newCandidateFound=false;
    if (distToNearestLngScale==-1) {
      newCandidateFound=true;
    } else {
      if ((distToLngScale<distToNearestLngScale)&&(distToLatScale<distToNearestLatScale)) {
        newCandidateFound=true;
      }
    }
    if (newCandidateFound) {
      distToNearestLngScale=distToLngScale;
      distToNearestLatScale=distToLatScale;
      z=i;
    }
  }
  return z;
}

// Creates the calibrator for the given bounds
MapCalibrator *MapSourceMercatorTiles::createMapCalibrator(double latNorth, double latSouth, double lngWest, double lngEast) {
  MapCalibrator *mapCalibrator=MapCalibrator::newMapCalibrator(MapCalibratorTypeMercator);
  if (!mapCalibrator) {
    FATAL("can not create map calibrator",NULL);
    return NULL;
  }
  MapPosition t;
  t.setX(0);
  t.setY(0);
  t.setLat(latNorth);
  t.setLng(lngWest);
  mapCalibrator->addCalibrationPoint(t);
  t.setX(mapTileLength-1);
  t.setLng(lngEast);
  mapCalibrator->addCalibrationPoint(t);
  t.setY(mapTileLength-1);
  t.setLat(latSouth);
  mapCalibrator->addCalibrationPoint(t);
  t.setX(0);
  t.setLng(lngWest);
  mapCalibrator->addCalibrationPoint(t);
  return mapCalibrator;
}

// Fetches the map tile in which the given position lies from disk or server
MapTile *MapSourceMercatorTiles::fetchMapTile(MapPosition pos, Int zoomLevel) {

  // Do not continue if an error has occured
  if (errorOccured)
    return NULL;

  // Check if the position is within the allowed bounds
  if ((pos.getLng()>MapSourceMercatorTiles::lngBound)||(pos.getLng()<-MapSourceMercatorTiles::lngBound))
    return NULL;
  if ((pos.getLat()>MapSourceMercatorTiles::latBound)||(pos.getLat()<-MapSourceMercatorTiles::latBound))
    return NULL;

  // Decide on the zoom level
  Int z;
  if (zoomLevel!=0) {
    z=minZoomLevel+zoomLevel-1;
  } else {
    z=findBestMatchingZoomLevel(pos);
  }
  if (z>maxZoomLevel) {
    z=maxZoomLevel;
  }
  Int translatedZ=z-minZoomLevel+1;

  // Compute the tile numbers
  Int x,y,max;
  pos.computeMercatorTileXY(z,x,y);
  max=(1<<z)-1;
  if ((x<0)||(x>max))
    return NULL;
  if ((y<0)||(y>max))
    return NULL;

  // Compute the bounds of this tile
  double latSouth, latNorth, lngEast, lngWest;
  pos.computeMercatorTileBounds(z,latNorth,latSouth,lngWest,lngEast);

  // Check if the container is not already available
  MapPosition t;
  t.setLat(latSouth+(latNorth-latSouth)/2);
  t.setLng(lngWest+(lngEast-lngWest)/2);
  MapTile *mapTile = MapSource::findMapTileByGeographicCoordinate(t,translatedZ,true,NULL);
  if (mapTile)
    return mapTile;

#ifdef TARGET_LINUX
  // Backup check to find problems
  for(std::vector<MapContainer*>::iterator i=mapContainers.begin();i!=mapContainers.end();i++) {
    MapContainer *c=*i;
    if ((c->getX()==x)&&(c->getY()==y)&&(c->getZoomLevel()==translatedZ)) {
      FATAL("tile alreay available",NULL);
      return c->getMapTiles()->front();
    }
  }
#endif

  // Prepare the filenames
  std::stringstream imageFileFolder; imageFileFolder << getFolderPath() << "/" << z << "/" << x;
  std::stringstream imageFileBase; imageFileBase << z << "_" << x << "_" << y;
  std::string imageFileExtension = tileServerURL.substr(tileServerURL.find_last_of(".")+1);
  std::string imageFileName = imageFileBase.str() + "." + imageFileExtension;
  std::string imageFilePath = imageFileFolder.str() + "/" + imageFileName;
  std::string calibrationFileName = imageFileBase.str() + ".gdm";
  std::string calibrationFilePath = imageFileFolder.str() + "/" + calibrationFileName;

  // Check if the image is supported
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
    ERROR("unsupported image file extension <%s> in tile server url %s",imageFileExtension.c_str(),tileServerURL.c_str());
    errorOccured=true;
    return NULL;
  }

  // Create a new map container
  MapContainer *mapContainer=new MapContainer();
  if (!mapContainer) {
    FATAL("can not create map container",NULL);
    return NULL;
  }
  mapContainer->setMapFileFolder(imageFileFolder.str());
  mapContainer->setImageFileName(imageFileName);
  mapContainer->setCalibrationFileName(calibrationFileName);
  mapContainer->setZoomLevel(translatedZ);
  mapContainer->setWidth(mapTileLength);
  mapContainer->setHeight(mapTileLength);
  mapContainer->setImageType(imageType);
  mapContainer->setX(x);
  mapContainer->setY(y);
  mapContainer->setOverlayGraphicInvalid(true);

  // Create the map calibrator
  MapCalibrator *mapCalibrator=createMapCalibrator(latNorth,latSouth,lngWest,lngEast);
  if (!mapCalibrator) {
    FATAL("can not create map calibrator",NULL);
    delete mapContainer;
    return NULL;
  }
  mapContainer->setMapCalibrator(mapCalibrator);

  // Create the map tile
  mapTile=new MapTile(0,0,mapContainer);
  if (!mapTile) {
    FATAL("can not create map tile",NULL);
    delete mapContainer;
    return NULL;
  }
  mapContainer->addTile(mapTile);

  // Update the search structures
  mapContainer->createSearchTree();

  // Check if the tile has already been saved to disk
  if (access(imageFilePath.c_str(),F_OK)) {

    // Flag that the image of the map container is not yet there
    mapContainer->setDownloadComplete(false);

    // Request the download thread to fetch this image
    core->getThread()->lockMutex(downloadQueueMutex);
    downloadQueue.push_back(mapContainer);
    core->getThread()->unlockMutex(downloadQueueMutex);
    core->getThread()->issueSignal(downloadStartSignal);

  }

  // Store the new map container and indicate that a search data structure is required
  mapContainers.push_back(mapContainer);
  insertNodeIntoSearchTree(mapContainer,mapContainer->getZoomLevel(),NULL,false,GeographicBorderLatNorth);
  insertNodeIntoSearchTree(mapContainer,0,NULL,false,GeographicBorderLatNorth);
  contentsChanged=true;

  // Request the cache to add this tile
  core->getMapCache()->addTile(mapTile);

  // Request the navigation engine to add overlays to the new tile
  core->getNavigationEngine()->addGraphics(mapContainer);

  // Return the tile
  return mapTile;
}

// Returns the map tile in which the position lies
MapTile *MapSourceMercatorTiles::findMapTileByGeographicCoordinate(MapPosition pos, Int zoomLevel, bool lockZoomLevel, MapContainer *preferredMapContainer) {

  // First check if tile is in the cache
  MapTile *result = MapSource::findMapTileByGeographicCoordinate(pos,zoomLevel,lockZoomLevel,preferredMapContainer);
  if (!result) {

    // No, let's fetch it from the server or disk
    return fetchMapTile(pos,zoomLevel);

  } else {

    // Check that there is not one on the disk/server that better matches the scale
    if (!lockZoomLevel) {
      Int newZoomLevel = findBestMatchingZoomLevel(pos)-minZoomLevel+1;
      if (newZoomLevel!=result->getParentMapContainer()->getZoomLevel()) {
        return fetchMapTile(pos,newZoomLevel);
      }
    }
    return result;
  }
}

// Returns the map tile that lies in a given area
MapTile *MapSourceMercatorTiles::findMapTileByGeographicArea(MapArea area, MapTile *preferredNeigbor, MapContainer* &usedMapContainer) {

  // First check if tile is in the cache
  MapTile *result = MapSource::findMapTileByGeographicArea(area,preferredNeigbor,usedMapContainer);
  if (!result) {

    // Respect the bounds
    if (area.getLatNorth()>latBound)
      area.setLatNorth(latBound);
    if (area.getLatSouth()<-latBound)
      area.setLatSouth(-latBound);
    if (area.getLngEast()>lngBound)
      area.setLngEast(lngBound);
    if (area.getLngWest()<-lngBound)
      area.setLngWest(-lngBound);

    // No, let's fetch it from the server or disk
    return fetchMapTile(area.getCenterPos(),area.getZoomLevel());

  } else {
    return result;
  }
}

// Performs maintenance (e.g., recreate degraded search tree)
void MapSourceMercatorTiles::maintenance() {

  lockAccess();

  // Was the source modified?
  if (contentsChanged) {

    // Remove map containers from list until cache size is reached again
    for(std::vector<MapContainer*>::iterator i=mapContainers.begin();i!=mapContainers.end();i++) {
      if (mapContainers.size()>mapContainerCacheSize) {

        // Remove container if it is not currently downloaded and if it is not visible
        MapContainer *c=*i;
        if ((!c->isDrawn())&&(c->getDownloadComplete()&&(!c->getOverlayGraphicInvalid()))) {
          core->getMapCache()->removeTile(c->getMapTiles()->front());
          core->getNavigationEngine()->removeGraphics(c);
          mapContainers.erase(i);
          delete c;
        }

      } else {
        break;
      }
    }
    if (mapContainers.size()>mapContainerCacheSize) {
      WARNING("map container list could not be reduced below %d entries",mapContainerCacheSize);
    }

    // Recreate the search tree
    createSearchDataStructures();

  }

  // Reset the download warning message
  downloadWarningOccured=false;

  unlockAccess();
}

// Downloads map tiles from the tile server
void MapSourceMercatorTiles::downloadMapImages() {

  std::list<std::string> status;

  // Set the priority
  core->getThread()->setThreadPriority(threadPriorityBackgroundLow);

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
      setStatus(status);

      // Prepare the directory
      std::stringstream t;
      std::stringstream z; z << mapContainer->getZoomLevel()+minZoomLevel-1;
      std::stringstream x; x << mapContainer->getX();
      std::stringstream y; y << mapContainer->getY();
      t << getFolderPath() << "/" << z.str();
      if (!createMapFolder(t.str()))
        continue;
      t << "/" << x.str();
      if (!createMapFolder(t.str()))
        continue;

      // Prepare the url
      std::string url=tileServerURL;
      replaceVariableInTileServerURL(url,"${z}",z.str());
      replaceVariableInTileServerURL(url,"${x}",x.str());
      replaceVariableInTileServerURL(url,"${y}",y.str());

      // Download the image
      bool downloadSuccess=downloadMapImage(url,mapContainer->getImageFilePath());
      bool maxRetriesReached=(mapContainer->getDownloadRetries()>=maxDownloadRetries);
      if (downloadSuccess||maxRetriesReached) {

        // Write the gdm file
        if (downloadSuccess)
          mapContainer->writeCalibrationFile();
        else {
          WARNING("aborting download of <%s>",url.c_str());
        }


        // Update the map cache
        lockAccess();
        mapContainer->setDownloadComplete(true);
        unlockAccess();
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
      setStatus(status);

      // Shall we quit?
      if (quitMapImageDownloadThread) {
        core->getThread()->exitThread();
      }
    }
  }
}

// Finds the calibrator for the given position
MapCalibrator *MapSourceMercatorTiles::findMapCalibrator(Int zoomLevel, MapPosition pos, bool &deleteCalibrator) {
  deleteCalibrator=false;
  MapTile *tile=MapSource::findMapTileByGeographicCoordinate(pos,zoomLevel,true,NULL);
  if (tile) {
    deleteCalibrator=false;
    return tile->getParentMapContainer()->getMapCalibrator();
  } else {
    deleteCalibrator=true;
    Int z=zoomLevel-1+minZoomLevel;
    double latNorth, latSouth, lngWest, lngEast;
    pos.computeMercatorTileBounds(z,latNorth,latSouth,lngWest,lngEast);
    return createMapCalibrator(latNorth,latSouth,lngWest,lngEast);
  }
}

// Returns the scale values for the given zoom level
void MapSourceMercatorTiles::getScales(Int zoomLevel, double &latScale, double &lngScale) {
  MapPosition pos;
  pos.setLat(latBound);
  pos.setLng(0);
  double latNorth,latSouth,lngWest,lngEast;
  pos.computeMercatorTileBounds(zoomLevel-1+minZoomLevel,latNorth,latSouth,lngWest,lngEast);
  lngScale=mapTileLength/(lngEast-lngWest);
  latScale=mapTileLength/(latNorth-latSouth);
}

// Downloads a map image from the server
bool MapSourceMercatorTiles::downloadMapImage(std::string url, std::string filePath) {

  std::string tempFilePath = getFolderPath() + "/mapSourceDownloadBuffer.bin";
  Int imageWidth, imageHeight;

  // Download the file
  if (!(core->downloadURL(url,tempFilePath,!downloadWarningOccured))) {
    downloadWarningOccured=true;
    return false;
  } else {

    // Check if the image can be loaded
    if ((!core->getImage()->queryPNG(tempFilePath,imageWidth,imageHeight))&&
       (!core->getImage()->queryJPEG(tempFilePath,imageWidth,imageHeight))) {

      WARNING("downloaded map from <%s> is not a valid image",url.c_str());
      return false;

    } else {

      // Move the file to its intended position
      rename(tempFilePath.c_str(),filePath.c_str());
      return true;

    }

  }

}

} /* namespace GEODISCOVERER */
