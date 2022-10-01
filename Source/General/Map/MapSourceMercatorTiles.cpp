//============================================================================
// Name        : MapSourceMercatorTiles.cpp
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
#include <MapSourceMercatorTiles.h>
#include <MapPosition.h>
#include <MapCache.h>
#include <NavigationEngine.h>
#include <MapEngine.h>
#include <NavigationPath.h>
#include <UnitConverter.h>
#include <Commander.h>

// Executes an command on the java side
std::string GDApp_executeAppCommand(std::string command);

namespace GEODISCOVERER {

// Constant values
const double MapSourceMercatorTiles::latBound = 85.0511287798;
const double MapSourceMercatorTiles::lngBound = 180.0;

// Process downloads thread
void *mapSourceMercatorTilesProcessDownloadJobsThread(void *args) {
  MapSourceMercatorTiles *mapSource = (MapSourceMercatorTiles*) args;
  mapSource->processDownloadJobs();
  return NULL;
}

// Constructor
MapSourceMercatorTiles::MapSourceMercatorTiles(TimestampInSeconds lastGDSModification) : MapSource() {
  type=MapSourceTypeMercatorTiles;
  mapContainerCacheSize=core->getConfigStore()->getIntValue("Map","mapContainerCacheSize",__FILE__, __LINE__);
  downloadAreaLength=core->getConfigStore()->getIntValue("Map","downloadAreaLength",__FILE__, __LINE__) * 1000;
  downloadAreaMinDistance=core->getConfigStore()->getIntValue("Map","downloadAreaMinDistance",__FILE__, __LINE__) * 1000;
  mapFolderDiskUsage=0;
  mapFolderMaxSize=((Long)core->getConfigStore()->getIntValue("Map","mapFolderMaxSize",__FILE__, __LINE__))*1024LL*1024LL;
  mapTileLength=256;
  accessMutex=core->getThread()->createMutex("map source mercator tiles access mutex");
  errorOccured=false;
  downloadWarningOccured=false;
  mapDownloader=new MapDownloader(this);
  if (mapDownloader==NULL)
    FATAL("can not create map downloader object",NULL);
  downloadJobsMutex=core->getThread()->createMutex("map source mercator tiles download jobs mutex");
  processDownloadJobsThreadMutex=core->getThread()->createMutex("map source mercator tiles process download jobs thread mutex");
  quitProcessDownloadJobsThread=false;
  processDownloadJobsThreadInfo=NULL;
  unqueuedDownloadTileCount=0;
  this->lastGDSModification=lastGDSModification;
  this->lastGDMModification=0;
}

// Destructor
MapSourceMercatorTiles::~MapSourceMercatorTiles() {

  // Stop the map downloader object
  DEBUG("deleting map downloader",NULL);
  delete mapDownloader;
  mapDownloader=NULL;

  // Free everything else
  deinit();
  core->getThread()->destroyMutex(processDownloadJobsThreadMutex);
  core->getThread()->destroyMutex(accessMutex);
  core->getThread()->destroyMutex(downloadJobsMutex);
}

// Deinitializes the map source
void MapSourceMercatorTiles::deinit() {
  DEBUG("deinit map source",NULL);
  MapSource::deinit();
}

// Initializes the map source
bool MapSourceMercatorTiles::init() {

  std::string mapPath=getFolderPath();
  ZipArchive *mapArchive;

  // Read the information from the config file
  if (!parseGDSInfo())
    return false;

  // We always need the default tiles.gda file
  lockMapArchives(__FILE__, __LINE__);
  if (!(mapArchive=new ZipArchive(mapPath,"tiles.gda"))) {
    FATAL("can not create zip archive object",NULL);
    return false;
  }
  if (!mapArchive->init()) {
    ERROR("can not open tiles.gda in map directory <%s>",folder.c_str());
    return false;
  }
  mapArchives.push_back(mapArchive);

  // Read all the additional tilesX.gda files
  std::string title="Reading tiles of map " + getFolder();
  DialogKey dialog=core->getDialog()->createProgress(title,0);
  struct dirent *dp;
  DIR *dfd;
  dfd=core->openDir(mapPath);
  if (dfd==NULL) {
    FATAL("can not read directory <%s>",mapPath.c_str());
    return false;
  }
  while ((dp = readdir(dfd)) != NULL)
  {
    Int nr;

    // Remove any left over write tries
    std::string filename(dp->d_name);
    if ((filename.find(".gda.")!=std::string::npos)&&(filename.find("tiles")!=std::string::npos)) {
      remove((mapPath + "/" + filename).c_str());
    } else {

      /* Add this archive if it is valid
      if (sscanf(dp->d_name,"tiles%d.gda",&nr)==1) {
        ZipArchive *archive = new ZipArchive(mapPath,dp->d_name);
        if ((archive)&&(archive->init())) {
          mapArchives.push_back(archive);
        }
      }*/

      // Delete any archive (not used anymore)
      if ((sscanf(dp->d_name,"tiles%d.gda",&nr)==1)||(strcmp(dp->d_name,"tiles.gda")==0)) {
        std::string p = mapPath+"/"+dp->d_name;
        //DEBUG("removing <%s> (not used anymore)",p.c_str());
        unlink(p.c_str());
      }
    }
  }
  closedir(dfd);
  core->getDialog()->closeProgress(dialog);
  unlockMapArchives();

  // Init the map downloader
  if (!mapDownloader->init())
    return false;

  // Init the center position
  centerPosition=new MapPosition();
  if (!centerPosition) {
    FATAL("can not create map position object",NULL);
  }
  centerPosition->setLat(0);
  centerPosition->setLng(0);
  centerPosition->setLatScale(((double)mapTileLength*pow(2.0,(double)0))/2.0/MapSourceMercatorTiles::latBound);
  centerPosition->setLngScale(((double)mapTileLength*pow(2.0,(double)0))/2.0/MapSourceMercatorTiles::lngBound);

  // Rename the layers
  renameLayers();

  // Init the zoom levels
  for (int z=0;z<(maxZoomLevel-minZoomLevel)+2;z++) {
    zoomLevelSearchTrees.push_back(NULL);
  }

  // Finished
  isInitialized=true;
  return true;
}

// Finds the best matching zoom level
Int MapSourceMercatorTiles::findBestMatchingZoomLevel(MapPosition pos, Int refZoomLevelMap, Int &minZoomLevelMap, Int &minZoomLevelServer) {
  double distToNearestLngScale=-1,distToNearestLatScale;
  Int z=minZoomLevelServer;
  std::list<MapTileServer*> *tileServers=mapDownloader->getTileServers();
  for (std::list<MapTileServer*>::iterator j=tileServers->begin();j!=tileServers->end();j++) {
    MapTileServer *tileServer=*j;
    double latSouth, latNorth, lngEast, lngWest;
    double distToLngScale,distToLatScale;
    double lngScale,latScale;
    bool newCandidateFound;
    if ((refZoomLevelMap==std::numeric_limits<Int>::min())||((refZoomLevelMap>=tileServer->getMinZoomLevelMap())&&(refZoomLevelMap<=tileServer->getMaxZoomLevelMap()))) {
      for (int i=tileServer->getMinZoomLevelServer();i<=tileServer->getMaxZoomLevelServer();i++) {
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
          minZoomLevelMap=tileServer->getMinZoomLevelMap();
          minZoomLevelServer=tileServer->getMinZoomLevelServer();
          z=i-minZoomLevelServer+minZoomLevelMap;
        }
      }
    }
  }
  return z;
}

// Creates the calibrator for the given bounds
MapCalibrator *MapSourceMercatorTiles::createMapCalibrator(double latNorth, double latSouth, double lngWest, double lngEast) {
  MapCalibrator *mapCalibrator=MapCalibrator::newMapCalibrator(MapCalibratorTypeSphericalNormalMercator);
  if (!mapCalibrator) {
    FATAL("can not create map calibrator",NULL);
    return NULL;
  }
  mapCalibrator->init();
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

// Crestes the path for the given zoom and x
void MapSourceMercatorTiles::createTilePath(Int zMap, Int x, Int y, std::stringstream &archiveFileFolder, std::stringstream &archiveFileBase) {
  archiveFileFolder << getFolderPath() << "/" << "Tiles";
  struct stat s;
  Int result;
  if ((result=core->statFile(archiveFileFolder.str(), &s) != 0) || (!(s.st_mode & S_IFDIR))) {
    //DEBUG("path=%s result=%d errno=%d",path.c_str(),result,errno);
    if (mkdir(archiveFileFolder.str().c_str(), S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
      FATAL("can not create directory <%s>", archiveFileFolder.str().c_str());
    }
  }
  archiveFileFolder << "/" << zMap;
  if ((result=core->statFile(archiveFileFolder.str(), &s) != 0) || (!(s.st_mode & S_IFDIR))) {
    //DEBUG("path=%s result=%d errno=%d",path.c_str(),result,errno);
    if (mkdir(archiveFileFolder.str().c_str(), S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
      FATAL("can not create directory <%s>", archiveFileFolder.str().c_str());
    }
  }
  archiveFileFolder << "/" << x;
  if ((result=core->statFile(archiveFileFolder.str(), &s) != 0) || (!(s.st_mode & S_IFDIR))) {
    //DEBUG("path=%s result=%d errno=%d",path.c_str(),result,errno);
    if (mkdir(archiveFileFolder.str().c_str(), S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
      FATAL("can not create directory <%s>", archiveFileFolder.str().c_str());
    }
  }
  archiveFileBase << zMap << "_" << x << "_" << y;
}

// Fetches the map tile in which the given position lies from disk or server
MapTile *MapSourceMercatorTiles::fetchMapTile(MapPosition pos, Int zoomLevel) {

  if (accessMutex->lockedCount==0) {
    FATAL("accessMutex of MapSourceMercatorTiles must be locked",NULL);
  }

  // Do not continue if an error has occured
  if (errorOccured)
    return NULL;

  // Check if the position is within the allowed bounds
  if ((pos.getLng()>MapSourceMercatorTiles::lngBound)||(pos.getLng()<-MapSourceMercatorTiles::lngBound))
    return NULL;
  if ((pos.getLat()>MapSourceMercatorTiles::latBound)||(pos.getLat()<-MapSourceMercatorTiles::latBound))
    return NULL;

  // Decide on the zoom level
  Int zMap,minZoomLevelMap,minZoomLevelServer;
  if (zoomLevel!=0) {
    zMap=minZoomLevel+zoomLevel-1;
    findBestMatchingZoomLevel(pos,zMap,minZoomLevelMap,minZoomLevelServer);
  } else {
    zMap=findBestMatchingZoomLevel(pos,std::numeric_limits<Int>::min(),minZoomLevelMap,minZoomLevelServer);
  }
  if (zMap>maxZoomLevel) {
    zMap=maxZoomLevel;
  }
  Int zServer=zMap-minZoomLevelMap+minZoomLevelServer;
  //DEBUG("zServer=%d zMap=%d",zServer,zMap);

  // Compute the tile numbers
  Int x,y,max;
  pos.computeMercatorTileXY(zServer,x,y);
  max=(1<<zServer)-1;
  if ((x<0)||(x>max))
    return NULL;
  if ((y<0)||(y>max))
    return NULL;

  // Compute the bounds of this tile
  double latSouth, latNorth, lngEast, lngWest;
  pos.computeMercatorTileBounds(zServer,latNorth,latSouth,lngWest,lngEast);

  // Check if the container is not already available
  MapPosition t;
  t.setLat(latSouth+(latNorth-latSouth)/2);
  t.setLng(lngWest+(lngEast-lngWest)/2);
  MapTile *mapTile = MapSource::findMapTileByGeographicCoordinate(t,zMap,true,NULL);
  if (mapTile)
    return mapTile;

#ifdef TARGET_LINUX
  // Backup check to find problems
  for(std::vector<MapContainer*>::iterator i=mapContainers.begin();i!=mapContainers.end();i++) {
    MapContainer *c=*i;
    if ((c->getX()==x)&&(c->getY()==y)&&(c->getZoomLevelMap()==zMap)) {
      FATAL("tile alreay available",NULL);
      return c->getMapTiles()->front();
    }
  }
#endif

  // Prepare the filenames
  std::stringstream archiveFileBase;
  std::stringstream archiveFileFolder;
  createTilePath(zMap, x, y, archiveFileFolder, archiveFileBase);
  std::string imageFileExtension = "png";
  ImageType imageType = ImageTypePNG;
  std::string archiveFileName = archiveFileBase.str() + ".gda";
  std::string imageFileName = archiveFileBase.str() + "." + imageFileExtension;
  std::string calibrationFileName = archiveFileBase.str() + ".gdm";

  // Create a new map container
  MapContainer *mapContainer=new MapContainer();
  if (!mapContainer) {
    FATAL("can not create map container",NULL);
    return NULL;
  }
  mapContainer->setMapFileFolder(".");
  mapContainer->setArchiveFileFolder(archiveFileFolder.str());
  mapContainer->setArchiveFileName(archiveFileName);
  //DEBUG("archiveFileName(int)=%s archiveFileName=%s archiveFilePath=%s mapFileFolder=%s",archiveFileName.c_str(),mapContainer->getArchiveFileName().c_str(),mapContainer->getArchiveFilePath().c_str(),mapContainer->getMapFileFolder().c_str());
  mapContainer->setImageFileName(imageFileName);
  mapContainer->setImageFileName(imageFileName);
  mapContainer->setCalibrationFileName(calibrationFileName);
  mapContainer->setZoomLevelMap(zMap);
  mapContainer->setZoomLevelServer(zServer);
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
  if (access((mapContainer->getArchiveFilePath()).c_str(),F_OK)==-1) {
    mapDownloader->queueMapContainerDownload(mapContainer);
  }

  // Store the new map container and indicate that a search data structure is required
  mapContainers.push_back(mapContainer);
  insertNodeIntoSearchTree(mapContainer,mapContainer->getZoomLevelMap(),NULL,false,GeographicBorderLatNorth);
  insertNodeIntoSearchTree(mapContainer,0,NULL,false,GeographicBorderLatNorth);
  contentsChanged=true;

  // Request the cache to add this tile
  core->getMapCache()->addTile(mapTile);

  // Request the navigation engine to add overlays to the new tile
  core->getNavigationEngine()->addGraphics(mapContainer);

  //DEBUG("tile %s created",mapTile->getVisName().front().c_str());

  // Return the tile
  return mapTile;
}

// Returns the map tile in which the position lies
MapTile *MapSourceMercatorTiles::findMapTileByGeographicCoordinate(MapPosition pos, Int zoomLevel, bool lockZoomLevel, MapContainer *preferredMapContainer) {

  // First check if tile is in the cache
  MapTile *result = MapSource::findMapTileByGeographicCoordinate(pos,zoomLevel,lockZoomLevel,preferredMapContainer);
  if (!result) {

    // No, let's fetch it from the server or disk
    result = fetchMapTile(pos,zoomLevel);
  }

  // Check that there is not one on the disk/server that better matches the scale
  if (result!=NULL) {
    if (!lockZoomLevel) {
      Int minZoomLevelMap, minZoomLevelServer;
      Int newZoomLevelMap = findBestMatchingZoomLevel(pos,std::numeric_limits<Int>::min(),minZoomLevelMap,minZoomLevelServer);
      if (newZoomLevelMap!=result->getParentMapContainer()->getZoomLevelMap()) {
        return fetchMapTile(pos,newZoomLevelMap);
      }
    }
  }
  return result;
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
    MapPosition pos=area.getCenterPos();
    if (preferredNeigbor) {
      preferredNeigbor->getNeighborPos(area,pos);
    }
    return fetchMapTile(pos,area.getZoomLevel());

  } else {
    return result;
  }
}

// Marks a map container as obsolete
// Please note that other objects might still use this map container
// Call unlinkMapContainer to solve this afterwards
void MapSourceMercatorTiles::markMapContainerObsolete(MapContainer *c) {
  obsoleteMapContainers.push_back(c);
  for(std::vector<MapContainer*>::iterator i=mapContainers.begin();i!=mapContainers.end();i++) {
    if (*i==c) {
      mapContainers.erase(i);
      break;
    }
  }
}

// Computes the mercator x and y ranges for the given area and zoom level
void MapSourceMercatorTiles::computeMercatorBounds(MapArea *displayArea,Int zMap,Int &zServer,Int &startX,Int &endX,Int &startY,Int &endY) {
  Int minZoomLevelMap;
  Int minZoomLevelServer;
  Int maxZoomLevelServer;
  mapDownloader->getLayerGroupZoomLevelBounds(zMap,minZoomLevelMap,minZoomLevelServer,maxZoomLevelServer);
  zServer=zMap-minZoomLevelMap+minZoomLevelServer;
  MapPosition pos;
  pos.setLng(displayArea->getLngWest());
  pos.setLat(displayArea->getLatNorth());
  pos.computeMercatorTileXY(zServer,startX,startY);
  pos.setLng(displayArea->getLngEast());
  pos.setLat(displayArea->getLatSouth());
  pos.computeMercatorTileXY(zServer,endX,endY);
}

// Clears the given map directory
void MapSourceMercatorTiles::cleanMapFolder(std::string dirPath,MapArea *displayArea,bool allZoomLevels,bool removeAll, bool updateFileList) {

  // Go through the directory list
  DIR *dfd;
  struct dirent *dp;
  struct stat buffer;
  //DEBUG("dirPath=%s",dirPath.c_str());
  dfd=core->openDir(dirPath);
  if (dfd==NULL) {
    DEBUG("can not read directory <%s>",dirPath.c_str());
    return;
  }
  while ((dp = readdir(dfd)) != NULL) {

    // Process any new directory recursively
    std::string entry=dp->d_name;
    std::string entryPath = dirPath + "/" + entry;
    if (dp->d_type == DT_DIR) {
      if ((entry!=".")&&(entry!=".."))
        cleanMapFolder(entryPath,displayArea,allZoomLevels,removeAll,updateFileList);
    } else {

      // Is this left over from a write trial
      if (entry.find(".ack")!=std::string::npos) {
        remove(entryPath.c_str());
      }

      // Is this a gda file?
      std::string postfix=entry.substr(entry.size()-4,4);
      if (postfix==".gda") {
        Int z,x,y;
        std::istringstream s(entry),s2;
        std::string t;
        std::getline(s,t,'_');
        s2.clear(); s2.str(t);
        s2 >> z;
        std::getline(s,t,'_');
        s2.clear(); s2.str(t);
        s2 >> x;
        std::getline(s,t,'.');
        s2.clear(); s2.str(t);
        s2 >> y;

        // Find out the modification date
        struct stat stats;
        if (stat(entryPath.c_str(),&stats)!=0) {
          FATAL("can not read timestamp of <%s>",entryPath.c_str());
        }
        if (stats.st_mtime>lastGDMModification) 
          lastGDMModification=stats.st_mtime;
        //DEBUG("stats.st_size=%d",stats.st_size);

        // Shall we update the file list?
        if (updateFileList) {
          lockAccess(__FILE__,__LINE__);
          MapArchiveFile::updateFiles(&mapArchiveFiles,MapArchiveFile(entryPath,stats.st_mtime,stats.st_size));
          mapFolderDiskUsage+=stats.st_size;
          //DEBUG("adding map archive <%s> to file list (disk usage is %ld Bytes)",entryPath.c_str(),mapFolderDiskUsage);
          unlockAccess();
        }

        // Shall we remove it?
        if (displayArea) {
          MapPosition pos;
          Int startZoomLevel;
          Int endZoomLevel;
          if (allZoomLevels) {
            startZoomLevel=minZoomLevel;
            endZoomLevel=maxZoomLevel;
          } else {
            startZoomLevel=displayArea->getZoomLevel();
            endZoomLevel=displayArea->getZoomLevel();
          }
          for (Int zMap=startZoomLevel;zMap<=endZoomLevel;zMap++) {
            if (zMap==z) {
              Int zServer,startX,endX,startY,endY;
              computeMercatorBounds(displayArea,zMap,zServer,startX,endX,startY,endY);
              if ((x>=startX)&&(x<=endX)&&(y>=startY)&&(y<=endY)) {
                //DEBUG("deleting %s",entryPath.c_str());
                remove(entryPath.c_str());
              }
            }
          }
          recreateMapArchiveFiles=true;
        }
      }

      // Shall we remove everything?
      if (removeAll) {
        //DEBUG("deleting %s",entryPath.c_str());
        remove(entryPath.c_str());
        recreateMapArchiveFiles=true;
      }
    }
  }
  closedir(dfd);

  // Shall we delete the directory?
  if (removeAll) {
    //DEBUG("deleting all",NULL);
    remove(dirPath.c_str());
  }
}

// Removes all obsolete map containers
void MapSourceMercatorTiles::removeObsoleteMapContainers(MapArea *displayArea, bool allZoomLevels) {

  // Get a copy of the currently selected obsolete map containers
  lockAccess(__FILE__, __LINE__);
  std::list<MapContainer*> obsoleteMapContainers = this->obsoleteMapContainers;
  this->obsoleteMapContainers.clear();
  unlockAccess();

  // Recreate the search tree to get rid of all the references to obsolete map containers
  lockAccess(__FILE__, __LINE__);
  createSearchDataStructures();
  unlockAccess();

  // Delete the obsolete map containers
  for(std::list<MapContainer*>::iterator i=obsoleteMapContainers.begin();i!=obsoleteMapContainers.end();i++) {
    MapContainer *c=*i;
    //DEBUG("deleting map container 0x%08x",c);
    core->getMapCache()->removeTile(c->getMapTiles()->front());
    core->getNavigationEngine()->removeGraphics(c);
    delete c;
  }

  // Go through map directories recursively
  cleanMapFolder(getFolderPath() + "/Tiles",displayArea,allZoomLevels);
}

// Performs maintenance (e.g., recreate degraded search tree)
void MapSourceMercatorTiles::maintenance() {

  // Check the size of the map folder
  if (recreateMapArchiveFiles) {
    lockAccess(__FILE__,__LINE__);
    mapArchiveFiles.clear();
    mapFolderDiskUsage=0;
    unlockAccess();
  }
  cleanMapFolder(getFolderPath() + "/Tiles",NULL,false,false,recreateMapArchiveFiles);
  recreateMapArchiveFiles=false;

  // Remove the Tiles dir if the gds info is newer than the tiles
  DEBUG("map folder clean up started",NULL);
  if (lastGDSModification>lastGDMModification) {
    DialogKey key=core->getDialog()->createProgress("Removing all tiles (GDS info newer)",0);
    cleanMapFolder(getFolderPath() + "/Tiles",NULL,false,true);
    core->getDialog()->closeProgress(key);
  } else {

    // Reduce the folder size if necessary
    bool mapArchiveCandidatesLeft=true;
    while ((mapArchiveCandidatesLeft)&&(mapFolderDiskUsage>mapFolderMaxSize)&&(!core->getQuitCore())) {
      //DEBUG("mapArchiveFiles.size()=%d",mapArchiveFiles.size());
      std::list<MapArchiveFile>::iterator i;
      for (i=mapArchiveFiles.begin();i!=mapArchiveFiles.end();i++) {
        MapArchiveFile mapArchiveFile=*i;
        //DEBUG("mapArchiveFile=%s",mapArchiveFile.getFilePath().c_str());

        // Delete the oldest file unless it is in use
        lockAccess(__FILE__,__LINE__);
        bool mapArchiveFileInUse=false;
        for (std::vector<MapContainer*>::iterator j=mapContainers.begin();j!=mapContainers.end();j++) {
          MapContainer *mapContainer=*j;
          if (mapContainer->getArchiveFilePath()==mapArchiveFile.getFilePath()) {
            //DEBUG("map archive file <%s> is in use!",mapContainer->getArchiveFilePath().c_str());
            mapArchiveFileInUse=true;
            break;
          }
        }
        if (!mapArchiveFileInUse) {
          unlink(mapArchiveFile.getFilePath().c_str());
          mapFolderDiskUsage-=mapArchiveFile.getDiskUsage();
          //DEBUG("removed map archive <%s> (new disk usage: %d MB)",mapArchiveFile.getFilePath().c_str(),mapFolderDiskUsage/1024/1024);
        }
        unlockAccess();
        if (!mapArchiveFileInUse) {
          mapArchiveFiles.erase(i);
          break;
        }
      }
      if (i==mapArchiveFiles.end()) {
        //DEBUG("no further map folder size reduction possible, aborting",NULL);
        mapArchiveCandidatesLeft=false;
      }
    }
  }
  DEBUG("map folder cleanup finished (size: %ld MB)",mapFolderDiskUsage/1024/1024);

  // Was the source modified?
  if (contentsChanged) {

    // Remove map containers from the memory list until cache size is reached again
    lockAccess(__FILE__, __LINE__);
    for(std::vector<MapContainer*>::iterator i=mapContainers.begin();i!=mapContainers.end();) {
      if (mapContainers.size()>mapContainerCacheSize) {

        // Remove container if it is not currently downloaded and if it is not visible
        MapContainer *c=*i;
        if ((!c->isDrawn())&&(c->getDownloadComplete()&&(!c->getOverlayGraphicInvalid()))) {
          markMapContainerObsolete(c);
        } else {
          i++;
        }

      } else {
        break;
      }
    }
    contentsChanged=false;
    unlockAccess();

    // Remove the obsolete map containers
    removeObsoleteMapContainers();

  }

  // Reset the download warning message
  downloadWarningOccured=false;
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
    Int minZoomLevelMap;
    Int minZoomLevelServer;
    Int maxZoomLevelServer;
    mapDownloader->getLayerGroupZoomLevelBounds(zoomLevel,minZoomLevelMap,minZoomLevelServer,maxZoomLevelServer);
    Int zServer=zoomLevel-minZoomLevelMap+minZoomLevelServer;
    double latNorth, latSouth, lngWest, lngEast;
    pos.computeMercatorTileBounds(zServer,latNorth,latSouth,lngWest,lngEast);
    return createMapCalibrator(latNorth,latSouth,lngWest,lngEast);
  }
}

// Returns the scale values for the given zoom level
void MapSourceMercatorTiles::getScales(Int zoomLevel, double &latScale, double &lngScale) {
  MapPosition pos;
  pos.setLat(latBound);
  pos.setLng(0);
  Int minZoomLevelMap;
  Int minZoomLevelServer;
  Int maxZoomLevelServer;
  mapDownloader->getLayerGroupZoomLevelBounds(zoomLevel,minZoomLevelMap,minZoomLevelServer,maxZoomLevelServer);
  Int zServer=zoomLevel-minZoomLevelMap+minZoomLevelServer;
  double latNorth,latSouth,lngWest,lngEast;
  pos.computeMercatorTileBounds(zServer,latNorth,latSouth,lngWest,lngEast);
  lngScale=mapTileLength/(lngEast-lngWest);
  latScale=mapTileLength/(latNorth-latSouth);
}

// Reads information about the map
bool MapSourceMercatorTiles::parseGDSInfo() {
  bool result=false;
  std::string infoFilePath = getFolderPath() + "/info.gds";

  // Loop over the elements and extract the information
  std::list<XMLNode> tileServers=resolvedGDSInfo->findConfigNodes("/GDS/TileServer");
  for (std::list<XMLNode>::iterator i=tileServers.begin();i!=tileServers.end();i++) {
    std::string serverURL;
    XMLNode n=*i;
    bool serverURLFound=false;
    serverURLFound=resolvedGDSInfo->getNodeText(n,"serverURL",serverURL);
    double overlayAlpha;
    bool overlayAlphaFound=false;
    overlayAlphaFound=resolvedGDSInfo->getNodeText(n,"overlayAlpha",overlayAlpha);
    Int minZoomLevel;
    resolvedGDSInfo->getNodeText(n,"minZoomLevel",minZoomLevel);
    Int maxZoomLevel;
    resolvedGDSInfo->getNodeText(n,"maxZoomLevel",maxZoomLevel);
    std::string imageFormat="";
    resolvedGDSInfo->getNodeText(n,"imageFormat",imageFormat);
    std::string layerGroupName="";
    resolvedGDSInfo->getNodeText(n,"layerGroupName",layerGroupName);
    std::list<std::string> httpHeader;
    resolvedGDSInfo->getAllNodesText(n,"httpHeader",httpHeader);
    if (!serverURLFound) {
      ERROR("one TileServer element has no serverURL element in <%s>",infoFilePath.c_str());
      goto cleanup;
    }
    if (!overlayAlphaFound) {
      ERROR("one TileServer element has no overlayAlpha element in <%s>",infoFilePath.c_str());
      goto cleanup;
    }
    ImageType imageType = ImageTypeUnknown;
    if (imageFormat=="jpeg")
      imageType = ImageTypeJPEG;
    if (imageFormat=="png")
      imageType = ImageTypePNG;
    mapDownloader->addTileServer(serverURL,overlayAlpha,imageType,layerGroupName,minZoomLevel,maxZoomLevel,httpHeader);
  }
  if (tileServers.size()==0) {
    ERROR("no tileServer element found in <%s>",infoFilePath.c_str());
    goto cleanup;
  }

  mapDownloader->updateZoomLevels(minZoomLevel,maxZoomLevel,mapLayerNameMap);
  result=true;

cleanup:

  return result;
}

// Adds a download job from the current visible map
void MapSourceMercatorTiles::addDownloadJob(bool estimateOnly, std::string routeName, std::string zoomLevels) {

  // Get the display area to download
  MapArea area;
  if (routeName=="") {
    area = *(core->getMapEngine()->lockDisplayArea(__FILE__,__LINE__));
    core->getMapEngine()->unlockDisplayArea();
  }

  // Stop any ongoing download jobs
  stopDownloadJobProcessing();

  // Remove any estimation jobs
  core->getThread()->lockMutex(downloadJobsMutex,__FILE__,__LINE__);
  ConfigStore *c=core->getConfigStore();
  std::list<std::string> names=c->getAttributeValues("Map/DownloadJob","name",__FILE__,__LINE__);
  for (std::list<std::string>::iterator i=names.begin();i!=names.end();i++) {
    std::string configPath="Map/DownloadJob[@name='" + *i + "']";
    if (c->getIntValue(configPath,"estimateOnly",__FILE__,__LINE__)) {
      c->removePath(configPath);
      //DEBUG("removing %s",configPath.c_str());
    }
  }

  // Add new job
  std::string jobName=core->getClock()->getXMLDate(core->getClock()->getSecondsSinceEpoch(),true);
  //processedDownloadJobs.remove(jobName);
  std::string path="Map/DownloadJob[@name='" + jobName + "']";
  if (routeName=="") {
    c->setDoubleValue(path,"latNorth",area.getLatNorth(),__FILE__,__LINE__);
    c->setDoubleValue(path,"latSouth",area.getLatSouth(),__FILE__,__LINE__);
    c->setDoubleValue(path,"lngEast",area.getLngEast(),__FILE__,__LINE__);
    c->setDoubleValue(path,"lngWest",area.getLngWest(),__FILE__,__LINE__);
  } else {
    c->setStringValue(path,"routeName",routeName,__FILE__,__LINE__);
  }
  c->setStringValue(path,"zoomLevels",zoomLevels,__FILE__,__LINE__);
  c->setIntValue(path,"estimateOnly",estimateOnly,__FILE__,__LINE__);
  //DEBUG("jobName=%s",jobName.c_str());

  // Start the new job
  core->getThread()->unlockMutex(downloadJobsMutex);
  startDownloadJobProcessing();
}

// Stops the download job processing
void MapSourceMercatorTiles::stopDownloadJobProcessing() {

  // Stop any ongoing download jobs
  quitProcessDownloadJobsThread=true;
  core->getThread()->lockMutex(processDownloadJobsThreadMutex,__FILE__,__LINE__);
  if (processDownloadJobsThreadInfo) {
    core->getThread()->waitForThread(processDownloadJobsThreadInfo);
    core->getThread()->destroyThread(processDownloadJobsThreadInfo);
    processDownloadJobsThreadInfo=NULL;
  }
  core->getThread()->unlockMutex(processDownloadJobsThreadMutex);

}

// Starts the download job processing
void MapSourceMercatorTiles::startDownloadJobProcessing() {

  // Start the new job
  quitProcessDownloadJobsThread=false;
  core->getThread()->lockMutex(processDownloadJobsThreadMutex,__FILE__,__LINE__);
  processDownloadJobsThreadInfo=core->getThread()->createThread("map source mercator tiles process download jobs thread",mapSourceMercatorTilesProcessDownloadJobsThread,(void*)this);
  core->getThread()->unlockMutex(processDownloadJobsThreadMutex);
}

// Triggers the download job processing
void MapSourceMercatorTiles::triggerDownloadJobProcessing() {
  stopDownloadJobProcessing();
  startDownloadJobProcessing();
}

// Stops the download job processing
void MapSourceMercatorTiles::clearDownloadJobs() {
  DEBUG("stopping download job processing",NULL);
  stopDownloadJobProcessing();
  DEBUG("removing all download jobs",NULL);
  std::list<std::string> names=core->getConfigStore()->getAttributeValues("Map/DownloadJob","name",__FILE__,__LINE__);
  for (std::list<std::string>::iterator i=names.begin();i!=names.end();i++) {
    std::string configPath="Map/DownloadJob[@name='" + *i + "']";
    core->getConfigStore()->removePath(configPath);
  }
  DEBUG("clearing current download queue",NULL);
  mapDownloader->clearDownloadQueue();
}

// Ensures that all threads that download tiles are stopped
void MapSourceMercatorTiles::stopDownloadThreads() {
  DEBUG("stopping download job processing",NULL);
  stopDownloadJobProcessing();
  DEBUG("deinit map downloader",NULL);
  if (mapDownloader) mapDownloader->deinit();
}

// Processes all pending download jobs
void MapSourceMercatorTiles::processDownloadJobs() {

  core->getThread()->lockMutex(downloadJobsMutex,__FILE__,__LINE__);

  // Set the priority
  core->getThread()->setThreadPriority(threadPriorityBackgroundLow);

  // Get free storage space
  double freeStorageSpace=0;
  struct statvfs stat;
  if (statvfs(getFolderPath().c_str(),&stat)!=0) {
    FATAL("can not obtain free storage space",NULL);
    return;
  }
  freeStorageSpace=((double)stat.f_bsize)*((double)stat.f_bfree)/1024.0/1024.0;
  //DEBUG("free space: %lf MB",freeStorageSpace);

  // Go through all download jobs
  Int unqueuedDownloadTileCount=0;
  std::list<std::string> finishedDownloadJobs;
  ConfigStore *c=core->getConfigStore();
  double estimatedTotalStorageSpace=0;
  double averageMercatorTileSize=((double)c->getIntValue("Map","averageMercatorTileSize",__FILE__,__LINE__))/1024.0/1024.0;
  std::list<std::string> names=c->getAttributeValues("Map/DownloadJob","name",__FILE__,__LINE__);
  for (std::list<std::string>::iterator i=names.begin();i!=names.end();i++) {
    DEBUG("processing download job %s",(*i).c_str());

    // Set status
    std::list<std::string> status;
    status.clear();
    status.push_back("Processing download");
    status.push_back((*i).c_str());
    setStatus(status, __FILE__, __LINE__);

    // Process the download job
    double estimatedJobStorageSpace=0;
    bool allTilesDownloaded=true;
    std::string configPath="Map/DownloadJob[@name='" + *i + "']";
    bool estimateOnly=c->getIntValue(configPath,"estimateOnly",__FILE__,__LINE__);
    MapArea area;
    NavigationPath *route=NULL;
    std::string routeName = c->getStringValue(configPath,"routeName",__FILE__,__LINE__);
    if (routeName!="") {
      route=core->getNavigationEngine()->findRoute(routeName);
      if (route==NULL) {
        WARNING("removing download job because associated route <%s> can not be found",routeName.c_str());
        finishedDownloadJobs.push_back(*i);
        continue;
      }
    } else {
      double n;
      n=c->getDoubleValue(configPath,"latNorth",__FILE__,__LINE__);
      area.setLatNorth(n);
      n=c->getDoubleValue(configPath,"latSouth",__FILE__,__LINE__);
      area.setLatSouth(n);
      n=c->getDoubleValue(configPath,"lngEast",__FILE__,__LINE__);
      area.setLngEast(n);
      n=c->getDoubleValue(configPath,"lngWest",__FILE__,__LINE__);
      area.setLngWest(n);
    }
    std::istringstream s(c->getStringValue(configPath,"zoomLevels",__FILE__,__LINE__));
    std::string t;
    Int zMap;
    Int zMapNr=0;
    std::list<Int> zMapList;
    while (std::getline(s,t,',')) {
      //DEBUG("t=%s",t.c_str());
      MapLayerNameMap::iterator i=mapLayerNameMap.find(t);
      if (i==mapLayerNameMap.end()) {
        FATAL("can not find map layer <%s>", t.c_str());
      }
      zMapList.push_back(i->second);
    }
    double zMapStep = 100.0/(double)zMapList.size();
    for (std::list<Int>::iterator k=zMapList.begin();k!=zMapList.end();k++) {
      zMap=*k;
      //DEBUG("zMap=%d",zMap);

      // Go through the route points if this is a route
      std::list<MapArea> areas;
      if (route==NULL) {
        Int zServer,startX,endX,startY,endY;
        computeMercatorBounds(&area,zMap,zServer,startX,endX,startY,endY);
        area.setXWest(startX);
        area.setXEast(endX);
        area.setYNorth(startY);
        area.setYSouth(endY);
        area.setZoomLevel(zServer);
        //DEBUG("west: %3d north: %3d east: %3d south: %3d zoom: %3d",area.getXWest(),area.getYNorth(),area.getXEast(),area.getYSouth(),area.getZoomLevel());
        areas.push_back(area);
      } else {
        std::vector<MapPosition> mapPositions = route->getSelectedPoints();
        MapArea prevArea;
        bool storeCompletly = true;
        MapPosition prevMapPosition = NavigationPath::getPathInterruptedPos();
        for (std::vector<MapPosition>::iterator j=mapPositions.begin();j!=mapPositions.end();j++) {
          MapArea area;
          MapPosition mapPosition = *j;
          if (mapPosition!=NavigationPath::getPathInterruptedPos()) {
            if ((prevMapPosition==NavigationPath::getPathInterruptedPos())||(prevMapPosition.computeDistance(mapPosition)>=downloadAreaMinDistance)||(j+1==mapPositions.end())) {
              prevMapPosition=mapPosition;
              MapPosition t = mapPosition.computeTarget(0,downloadAreaLength/2);
              area.setLatNorth(t.getLat());
              t = mapPosition.computeTarget(90,downloadAreaLength/2);
              area.setLngEast(t.getLng());
              t = mapPosition.computeTarget(180,downloadAreaLength/2);
              area.setLatSouth(t.getLat());
              t = mapPosition.computeTarget(270,downloadAreaLength/2);
              area.setLngWest(t.getLng());
              Int zServer,startX,endX,startY,endY;
              computeMercatorBounds(&area,zMap,zServer,startX,endX,startY,endY);
              area.setXWest(startX);
              area.setXEast(endX);
              area.setYNorth(startY);
              area.setYSouth(endY);
              area.setZoomLevel(zServer);
              if (!storeCompletly) {

                // Check if no overlap with previous area
                if ((prevArea.getYNorth()>area.getYSouth())||(prevArea.getYSouth()<area.getYNorth())||
                    (prevArea.getXEast()<area.getXWest())||(prevArea.getXWest()>area.getXEast())) {
                  //DEBUG("area of new point is outside previous area, storing new area completely",NULL);
                  storeCompletly = true;
                }
              }
              if (storeCompletly) {

                // Store the complete area
                //DEBUG("west: %3d north: %3d east: %3d south: %3d zoom: %3d",area.getXWest(),area.getYNorth(),area.getXEast(),area.getYSouth(),area.getZoomLevel());
                areas.push_back(area);
                //DEBUG("lng=%f lat=%f",mapPosition.getLng(),mapPosition.getLat());
                //DEBUG("latNorth=%f latSouth=%f lngEast=%f lngWest=%f",area.getLatNorth(),area.getLatSouth(),area.getLngEast(),area.getLngWest());
                //DEBUG("horizontal length: %f vertical length: %f",eastPoint.computeDistance(westPoint),northPoint.computeDistance(southPoint));
                storeCompletly=false;

              } else {

                // Compute the non-overlapping area along the border
                MapArea area2;
                area2.setZoomLevel(area.getZoomLevel());
                if (area.getYNorth()<prevArea.getYNorth()) {
                  area2.setYNorth(area.getYNorth());
                  area2.setYSouth(prevArea.getYNorth()-1);
                  area2.setXWest(area.getXWest()>prevArea.getXWest()?area.getXWest():prevArea.getXWest());
                  area2.setXEast(area.getXEast());
                  //DEBUG("storing north stripe",NULL);
                  //DEBUG("west: %3d north: %3d east: %3d south: %3d zoom: %3d",area2.getXWest(),area2.getYNorth(),area2.getXEast(),area2.getYSouth(),area2.getZoomLevel());
                  areas.push_back(area2);
                }
                if (area.getXEast()>prevArea.getXEast()) {
                  area2.setXEast(area.getXEast());
                  area2.setXWest(prevArea.getXEast()+1);
                  area2.setYNorth(area.getYNorth()>prevArea.getYNorth()?area.getYNorth():prevArea.getYNorth());
                  area2.setYSouth(area.getYSouth());
                  //DEBUG("storing east stripe",NULL);
                  //DEBUG("west: %3d north: %3d east: %3d south: %3d zoom: %3d",area2.getXWest(),area2.getYNorth(),area2.getXEast(),area2.getYSouth(),area2.getZoomLevel());
                  areas.push_back(area2);
                }
                if (area.getYSouth()>prevArea.getYSouth()) {
                  area2.setYNorth(prevArea.getYSouth()+1);
                  area2.setYSouth(area.getYSouth());
                  area2.setXEast(area.getXEast()<prevArea.getXEast()?area.getXEast():prevArea.getXEast());
                  area2.setXWest(area.getXWest());
                  //DEBUG("storing south stripe",NULL);
                  //DEBUG("west: %3d north: %3d east: %3d south: %3d zoom: %3d",area2.getXWest(),area2.getYNorth(),area2.getXEast(),area2.getYSouth(),area2.getZoomLevel());
                  areas.push_back(area2);
                }
                if (area.getXWest()<prevArea.getXWest()) {
                  area2.setXEast(prevArea.getXWest()-1);
                  area2.setXWest(area.getXWest());
                  area2.setYNorth(area.getYNorth());
                  area2.setYSouth(area.getYSouth()<prevArea.getYSouth()?area.getYSouth():prevArea.getYSouth());
                  //DEBUG("storing west stripe",NULL);
                  //DEBUG("west: %3d north: %3d east: %3d south: %3d zoom: %3d",area2.getXWest(),area2.getYNorth(),area2.getXEast(),area2.getYSouth(),area2.getZoomLevel());
                  areas.push_back(area2);
                }
              }
              prevArea=area;
            }
          }
        }
      }

      // Go through all areas
      Int areaNr=0;
      for (std::list<MapArea>::iterator j=areas.begin();j!=areas.end();j++) {
        MapArea area=*j;
        status.pop_front();
        double progressValue;
        progressValue=round(zMapNr*zMapStep+areaNr*zMapStep/areas.size());
        std::stringstream progress; progress << "Processing download (" << (Int) progressValue << "%)";
        //DEBUG("progressValue=%f",progressValue);
        status.push_front(progress.str());
        setStatus(status, __FILE__, __LINE__);
        areaNr++;

        // Go through the tile ranges
        for (Int x=area.getXWest();x<=area.getXEast();x++) {
          for (Int y=area.getYNorth();y<=area.getYSouth();y++) {

            // Skip this if it is a estimate job and a quit is requested
            if ((quitProcessDownloadJobsThread)&&(estimateOnly))
              goto nextJob;

            // Tile not yet downloaded?
            std::stringstream fileFolder, fileBase;
            createTilePath(zMap, x, y, fileFolder, fileBase);
            std::string filePath =  fileFolder.str() + "/" + fileBase.str();
            if (access((filePath + ".gda").c_str(),F_OK)==-1) {
              allTilesDownloaded=false;
              //DEBUG("adding <%s.gda> to download queue",filePath.c_str());

              // Check if disk space is exceeded
              estimatedJobStorageSpace+=averageMercatorTileSize;
              if (!estimateOnly) {
                if (estimatedTotalStorageSpace+estimatedJobStorageSpace>freeStorageSpace) {
                  ERROR("suspending map download job because device has not enough free space (%d MB available)",freeStorageSpace);
                  goto nextJob;
                }
              }

              // Check if we shall quit
              if (core->getQuitCore())
                goto cleanup;

              // Queue it
              if (!estimateOnly) {

                // Only queue it if the queue is not too large
                if (!mapDownloader->downloadQueueReachedRecommendedSize()) {
                  MapPosition pos;
                  pos.setFromMercatorTileXY(area.getZoomLevel(),x,y);
                  lockAccess(__FILE__,__LINE__);
                  fetchMapTile(pos,zMap);
                  unlockAccess();
                } else {
                  unqueuedDownloadTileCount++;
                }
              }
            }
          }
        }
      }
      zMapNr++;
    }

nextJob:

    // Remember size of this job
    estimatedTotalStorageSpace+=estimatedJobStorageSpace;

    // Update app if this is an estimate job
    if ((estimateOnly)&&(!quitProcessDownloadJobsThread)) {
      std::stringstream cmd;
      std::string value,unit;
      cmd << "updateDownloadJobSize(";
      core->getUnitConverter()->formatBytes(estimatedJobStorageSpace*1024.0*1024.0,value,unit);
      cmd << "\"" << value << " " << unit << "\",\"";
      core->getUnitConverter()->formatBytes(freeStorageSpace*1024.0*1024.0,value,unit);
      cmd << value << " " << unit << "\",";
      if (estimatedJobStorageSpace>freeStorageSpace)
        cmd << "1";
      else
        cmd << "0";
      cmd << ")";
      //DEBUG("%s",cmd.str().c_str());
      core->getCommander()->dispatch(cmd.str());
    }

    // Remember this jobs if it's finished
    if ((allTilesDownloaded)||(estimateOnly))
      finishedDownloadJobs.push_back(*i);

    // Clear status
    status.clear();
    setStatus(status, __FILE__, __LINE__);

  }
  //DEBUG("estimatedStorageSpace: %lf MB",estimatedTotalStorageSpace);
  this->unqueuedDownloadTileCount=unqueuedDownloadTileCount;
  //DEBUG("number of to be downloaded but not queued tiles: %d",unqueuedDownloadTileCount);

cleanup:

  // Remove download job if all tiles have been downloaded
  for (std::list<std::string>::iterator i=finishedDownloadJobs.begin();i!=finishedDownloadJobs.end();i++) {
    std::string configPath="Map/DownloadJob[@name='" + *i + "']";
    c->removePath(configPath);
    //DEBUG("removed %s",configPath.c_str());
    //processedDownloadJobs.remove(*i);
  }

  core->getThread()->unlockMutex(downloadJobsMutex);
}

// Returns the number of unqueued but not downloaded tiles
Int MapSourceMercatorTiles::countUnqueuedDownloadTiles() {
  return unqueuedDownloadTileCount;
}

// Suspends the download job processing
void MapSourceMercatorTiles::lockDownloadJobProcessing(const char *file, int line) {
  core->getThread()->lockMutex(downloadJobsMutex,file,line);
}

// Continues the download job processing
void MapSourceMercatorTiles::unlockDownloadJobProcessing() {
  core->getThread()->unlockMutex(downloadJobsMutex);
}

// Indicates if the map source has download jobs
bool MapSourceMercatorTiles::hasDownloadJobs() {
  std::list<std::string> names=core->getConfigStore()->getAttributeValues("Map/DownloadJob","name",__FILE__,__LINE__);
  if (names.size()>0)
    return true;
  else
    return false;
}

// Returns the map tile for the given mercator coordinates
MapTile *MapSourceMercatorTiles::fetchMapTile(Int z, Int x, Int y) {

  // First find out the zoom level to use
  Int zMap=-1;
  std::list<MapTileServer*> *tileServers=mapDownloader->getTileServers();
  for (std::list<MapTileServer*>::iterator j=tileServers->begin();j!=tileServers->end();j++) {
    MapTileServer *tileServer=*j;
    //DEBUG("minZoomLevelServer=%d maxZoomLevelServer=%d minZoomLevelMap=%d",tileServer->getMinZoomLevelServer(),tileServer->getMaxZoomLevelServer(),tileServer->getMinZoomLevelMap());
    if ((z>=tileServer->getMinZoomLevelServer())&&(z<=tileServer->getMaxZoomLevelServer())) {
      zMap=tileServer->getMinZoomLevelMap()+z-tileServer->getMinZoomLevelServer();
      break;
    }
  }
  if (zMap==-1)
    return NULL;
  //DEBUG("zServer=%d zMap=%d",z,zMap);

  // Compute the position from the found zoom level
  MapPosition pos;
  pos.setFromMercatorTileXY(z,x,y);

  // Then find the map tile
  return fetchMapTile(pos,zMap);
}

// Returns the zoom level of the server
Int MapSourceMercatorTiles::getServerZoomLevel(Int mapZoomLevel) {
  Int serverZoomLevel=mapZoomLevel;
  std::list<MapTileServer*> *tileServers=mapDownloader->getTileServers();
  for (std::list<MapTileServer*>::iterator j=tileServers->begin();j!=tileServers->end();j++) {
    MapTileServer *tileServer=*j;
    //DEBUG("minZoomLevelServer=%d maxZoomLevelServer=%d minZoomLevelMap=%d",tileServer->getMinZoomLevelServer(),tileServer->getMaxZoomLevelServer(),tileServer->getMinZoomLevelMap());
    if ((mapZoomLevel>=tileServer->getMinZoomLevelMap())&&(mapZoomLevel<=tileServer->getMaxZoomLevelMap())) {
      serverZoomLevel=tileServer->getMinZoomLevelServer()+mapZoomLevel-tileServer->getMinZoomLevelMap();
      break;
    }
  }
  return serverZoomLevel;
}

// Fetches a map tile and returns its image data
UByte *MapSourceMercatorTiles::fetchMapTile(Int z, Int x, Int y, double saturationOffset, double brightnessOffset, UInt &imageSize) {
  imageSize=0;
  UByte *imageData=NULL;
  lockAccess(__FILE__,__LINE__);
  if (getIsInitialized()) {
    MapSourceMercatorTiles *mapSource=(MapSourceMercatorTiles*)core->getMapSource();
    MapTile *mapTile=mapSource->fetchMapTile(z,x,y);
    if (mapTile!=NULL) {
      while (!mapTile->getParentMapContainer()->getDownloadComplete()) {          
        unlockAccess();
        usleep(1000);
        lockAccess(__FILE__,__LINE__);
      }
      if (!mapTile->getParentMapContainer()->getDownloadErrorOccured()) {
        ZipArchive *mapArchive = new ZipArchive(mapTile->getParentMapContainer()->getArchiveFileFolder(),mapTile->getParentMapContainer()->getArchiveFileName());
        if ((mapArchive==NULL)||(!mapArchive->init()))
          FATAL("can not create zip archive object",NULL);
        Int size=0;
        imageData=mapArchive->exportEntry(mapTile->getParentMapContainer()->getImageFilePath(),size);
        delete mapArchive;
        if (size>0) {
          imageSize=(UInt)size;
          if ((saturationOffset!=0)||(brightnessOffset!=0)) {
            UByte *imageData2=core->getImage()->hsvFilter(imageData,imageSize,0,saturationOffset,brightnessOffset);
            free(imageData);
            imageData=imageData2;
          }
        }
        //DEBUG("result=%s",result.c_str());*/
      } else {
        DEBUG("download error for tile (%d,%d,%d)",z,x,y);
      }
    } else {
      DEBUG("no map tile found at (%d,%d,%d)",z,x,y);
    }
  } else {
    ERROR("map source is not initialized",NULL);
  }
  unlockAccess();
  return imageData;
}

// Inserts the new map archive file into the file list
void MapSourceMercatorTiles::updateMapArchiveFiles(std::string filePath) {
  lockAccess(__FILE__,__LINE__);
  struct stat stats;
  if (stat(filePath.c_str(),&stats)!=0) {
    FATAL("can not read timestamp of <%s>",filePath.c_str());
  }
  //DEBUG("stats.st_size=%d",stats.st_size);
  MapArchiveFile::updateFiles(&mapArchiveFiles,MapArchiveFile(filePath,stats.st_mtime,stats.st_size));  
  mapFolderDiskUsage+=stats.st_size;
  //DEBUG("adding map archive <%s> to file list (disk usage is %ld Bytes)",filePath.c_str(),mapFolderDiskUsage);
  unlockAccess();
}


} /* namespace GEODISCOVERER */
