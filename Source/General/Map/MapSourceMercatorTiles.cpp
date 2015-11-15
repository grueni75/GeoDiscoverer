//============================================================================
// Name        : MapSourceMercatorTiles.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================getLayerGroupZoomLevelBounds================================================

#include <Core.h>

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
MapSourceMercatorTiles::MapSourceMercatorTiles() : MapSource() {
  type=MapSourceTypeMercatorTiles;
  mapContainerCacheSize=core->getConfigStore()->getIntValue("Map","mapContainerCacheSize",__FILE__, __LINE__);
  mapTileLength=256;
  accessMutex=core->getThread()->createMutex("map source mercator tiles access mutex");
  errorOccured=false;
  downloadWarningOccured=false;
  mapDownloader=new MapDownloader(this);
  if (mapDownloader==NULL)
    FATAL("can not create map downloader object",NULL);
  downloadJobsMutex=core->getThread()->createMutex("download jobs mutex");
  quitProcessDownloadJobsThread=false;
  processDonwloadJobsThreadInfo=NULL;
}

// Destructor
MapSourceMercatorTiles::~MapSourceMercatorTiles() {

  // Stop the map downloader object
  delete mapDownloader;
  mapDownloader=NULL;

  // Free everything else
  deinit();
  core->getThread()->destroyMutex(accessMutex);
  core->getThread()->destroyMutex(downloadJobsMutex);
}

// Deinitializes the map source
void MapSourceMercatorTiles::deinit() {
  if (processDonwloadJobsThreadInfo) {
    quitProcessDownloadJobsThread=true;
    core->getThread()->waitForThread(processDonwloadJobsThreadInfo);
    core->getThread()->destroyThread(processDonwloadJobsThreadInfo);
    processDonwloadJobsThreadInfo=NULL;
  }
  if (mapDownloader) mapDownloader->deinit();
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
  dfd=opendir(mapPath.c_str());
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

      // Add this archive if it is valid
      if (sscanf(dp->d_name,"tiles%d.gda",&nr)==1) {
        ZipArchive *archive = new ZipArchive(mapPath,dp->d_name);
        if ((archive)&&(archive->init())) {
          mapArchives.push_back(archive);
        }
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

  // Process the download jobs
  processDownloadJobs();

  // Finished
  isInitialized=true;
  return true;
}

// Finds the best matching zoom level
Int MapSourceMercatorTiles::findBestMatchingZoomLevel(MapPosition pos, Int refZoomLevelMap, Int &minZoomLevelMap, Int &minZoomLevelServer) {
  Int maxZoomLevelServer;
  mapDownloader->getLayerGroupZoomLevelBounds(refZoomLevelMap,minZoomLevelMap,minZoomLevelServer,maxZoomLevelServer);
  Int z=minZoomLevelServer;
  double latSouth, latNorth, lngEast, lngWest;
  double distToLngScale,distToLatScale;
  double distToNearestLngScale=-1,distToNearestLatScale;
  double lngScale,latScale;
  bool newCandidateFound;
  for (int i=minZoomLevelServer;i<=maxZoomLevelServer;i++) {
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
  return z-minZoomLevelServer+minZoomLevelMap;
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
    zMap=findBestMatchingZoomLevel(pos,minZoomLevel,minZoomLevelMap,minZoomLevelServer);
  }
  if (zMap>maxZoomLevel) {
    zMap=maxZoomLevel;
  }
  Int zServer=zMap-minZoomLevelMap+minZoomLevelServer;

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
  std::stringstream archiveFileFolder;
  archiveFileFolder << "Tiles";
  std::string path;
  path = getFolderPath() + "/" + archiveFileFolder.str();
  struct stat s;
  if ((stat(path.c_str(), &s)!=0)||(!(s.st_mode&S_IFDIR))) {
    if (mkdir(path.c_str(),S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
      FATAL("can not create directory <%s>",path.c_str());
    }
  }
  archiveFileFolder << "/" << zMap;
  path = getFolderPath() + "/" + archiveFileFolder.str();
  if ((stat(path.c_str(), &s)!=0)||(!(s.st_mode&S_IFDIR))) {
    if (mkdir(path.c_str(),S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
      FATAL("can not create directory <%s>",path.c_str());
    }
  }
  archiveFileFolder << "/" << x;
  path = getFolderPath() + "/" + archiveFileFolder.str();
  if ((stat(path.c_str(), &s)!=0)||(!(s.st_mode&S_IFDIR))) {
    if (mkdir(path.c_str(),S_IRWXU | S_IRWXG | S_IRWXO)!=0) {
      FATAL("can not create directory <%s>",path.c_str());
    }
  }
  std::stringstream archiveFileBase; archiveFileBase << zMap << "_" << x << "_" << y;
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
  mapContainer->setMapFileFolder(archiveFileFolder.str());
  mapContainer->setArchiveFileName(archiveFileName);
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
  if (access((getFolderPath() + "/" + mapContainer->getArchiveFilePath()).c_str(),F_OK)==-1) {
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
    return fetchMapTile(pos,zoomLevel);

  } else {

    // Check that there is not one on the disk/server that better matches the scale
    if (!lockZoomLevel) {
      Int minZoomLevelMap, minZoomLevelServer;
      Int newZoomLevelMap = findBestMatchingZoomLevel(pos,result->getParentMapContainer()->getZoomLevelMap(),minZoomLevelMap,minZoomLevelServer);
      if (newZoomLevelMap!=result->getParentMapContainer()->getZoomLevelMap()) {
        return fetchMapTile(pos,newZoomLevelMap);
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
void MapSourceMercatorTiles::cleanMapFolder(std::string dirPath,MapArea *displayArea,bool allZoomLevels) {

  // Go through the directory list
  DIR *dfd;
  struct dirent *dp;
  dfd=opendir(dirPath.c_str());
  if (dfd==NULL) {
    FATAL("can not read directory <%s>",dirPath.c_str());
  }
  while ((dp = readdir(dfd)) != NULL) {

    // Process any new directory recursively
    std::string entry=dp->d_name;
    std::string entryPath = dirPath + "/" + entry;
    if (dp->d_type == DT_DIR) {
      if ((entry!=".")&&(entry!=".."))
        cleanMapFolder(entryPath,displayArea,allZoomLevels);
    } else {

      // Is this left over from a write trial
      if (entry.find(".gda.")!=std::string::npos) {
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
        }
      }
    }
  }
  closedir(dfd);
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

  // Was the source modified?
  if (contentsChanged) {

    // Remove map containers from list until cache size is reached again
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
    bool serverURLFound=false;
    serverURLFound=resolvedGDSInfo->getNodeText(*i,"serverURL",serverURL);
    double overlayAlpha;
    bool overlayAlphaFound=false;
    overlayAlphaFound=resolvedGDSInfo->getNodeText(*i,"overlayAlpha",overlayAlpha);
    Int minZoomLevel;
    resolvedGDSInfo->getNodeText(*i,"minZoomLevel",minZoomLevel);
    Int maxZoomLevel;
    resolvedGDSInfo->getNodeText(*i,"maxZoomLevel",maxZoomLevel);
    std::string imageFormat="";
    resolvedGDSInfo->getNodeText(*i,"imageFormat",imageFormat);
    std::string layerGroupName="";
    resolvedGDSInfo->getNodeText(*i,"layerGroupName",layerGroupName);
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
    mapDownloader->addTileServer(serverURL,overlayAlpha,imageType,layerGroupName,minZoomLevel,maxZoomLevel);
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
void MapSourceMercatorTiles::addDownloadJob(bool estimateOnly, std::string zoomLevels) {

  // Get the display area to download
  MapArea area = *(core->getMapEngine()->lockDisplayArea(__FILE__,__LINE__));
  core->getMapEngine()->unlockDisplayArea();

  // Stop any ongoing download jobs
  quitProcessDownloadJobsThread=true;
  if (processDonwloadJobsThreadInfo) {
    core->getThread()->waitForThread(processDonwloadJobsThreadInfo);
    core->getThread()->destroyThread(processDonwloadJobsThreadInfo);
    processDonwloadJobsThreadInfo=NULL;
  }

  // Remove any estimation jobs
  core->getThread()->lockMutex(downloadJobsMutex,__FILE__,__LINE__);
  ConfigStore *c=core->getConfigStore();
  std::list<std::string> names=c->getAttributeValues("Map/DownloadJob","name",__FILE__,__LINE__);
  for (std::list<std::string>::iterator i=names.begin();i!=names.end();i++) {
    std::string configPath="Map/DownloadJob[@name='" + *i + "']";
    if (c->getIntValue(configPath,"estimateOnly",__FILE__,__LINE__)) {
      c->removePath(configPath);
    }
  }

  // Add new job
  std::string jobName=core->getClock()->getXMLDate(core->getClock()->getSecondsSinceEpoch(),true);
  processedDownloadJobs.remove(jobName);
  std::string path="Map/DownloadJob[@name='" + jobName + "']";
  c->setDoubleValue(path,"latNorth",area.getLatNorth(),__FILE__,__LINE__);
  c->setDoubleValue(path,"latSouth",area.getLatSouth(),__FILE__,__LINE__);
  c->setDoubleValue(path,"lngEast",area.getLngEast(),__FILE__,__LINE__);
  c->setDoubleValue(path,"lngWest",area.getLngWest(),__FILE__,__LINE__);
  c->setStringValue(path,"zoomLevels",zoomLevels,__FILE__,__LINE__);
  c->setIntValue(path,"estimateOnly",estimateOnly,__FILE__,__LINE__);
  //DEBUG("jobName=%s",jobName.c_str());

  // Start the new job
  quitProcessDownloadJobsThread=false;
  processDonwloadJobsThreadInfo=core->getThread()->createThread("map source mercator tiles process download jobs thread",mapSourceMercatorTilesProcessDownloadJobsThread,(void*)this);
  core->getThread()->unlockMutex(downloadJobsMutex);
}

// Processes all pending download jobs
void MapSourceMercatorTiles::processDownloadJobs() {

  core->getThread()->lockMutex(downloadJobsMutex,__FILE__,__LINE__);

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
  std::list<std::string> finishedDownloadJobs;
  ConfigStore *c=core->getConfigStore();
  double estimatedTotalStorageSpace=0;
  double averageMercatorTileSize=((double)c->getIntValue("Map","averageMercatorTileSize",__FILE__,__LINE__))/1024.0/1024.0;
  std::list<std::string> names=c->getAttributeValues("Map/DownloadJob","name",__FILE__,__LINE__);
  for (std::list<std::string>::iterator i=names.begin();i!=names.end();i++) {
    if (std::find(processedDownloadJobs.begin(), processedDownloadJobs.end(), *i)==processedDownloadJobs.end()) {
      //DEBUG("processing download job %s",(*i).c_str());
      processedDownloadJobs.push_back(*i);

      // Process the download job
      double estimatedJobStorageSpace=0;
      bool allTilesDownloaded=true;
      std::string configPath="Map/DownloadJob[@name='" + *i + "']";
      bool estimateOnly=c->getIntValue(configPath,"estimateOnly",__FILE__,__LINE__);
      MapArea area;
      double n;
      n=c->getDoubleValue(configPath,"latNorth",__FILE__,__LINE__);
      area.setLatNorth(n);
      n=c->getDoubleValue(configPath,"latSouth",__FILE__,__LINE__);
      area.setLatSouth(n);
      n=c->getDoubleValue(configPath,"lngEast",__FILE__,__LINE__);
      area.setLngEast(n);
      n=c->getDoubleValue(configPath,"lngWest",__FILE__,__LINE__);
      area.setLngWest(n);
      std::istringstream s(c->getStringValue(configPath,"zoomLevels",__FILE__,__LINE__));
      std::string t;
      Int zMap;
      while (std::getline(s,t,',')) {
        //DEBUG("t=%s",t.c_str());
        MapLayerNameMap::iterator i=mapLayerNameMap.find(t);
        if (i==mapLayerNameMap.end()) {
          FATAL("can not find map layer <%s>", t.c_str());
        }
        zMap=i->second;
        //DEBUG("zMap=%d",zMap);

        // Compute the tile ranges
        Int zServer,startX,endX,startY,endY;
        computeMercatorBounds(&area,zMap,zServer,startX,endX,startY,endY);

        // Go through the tile ranges
        for (Int x=startX;x<=endX;x++) {
          for (Int y=startY;y<=endY;y++) {

            // Tile not yet downloaded?
            std::stringstream filePath;
            filePath << getFolderPath() << "/Tiles/" << zMap << "/" << x << "/" << zMap << "_" << x << "_" << y << ".gda";
            //DEBUG("%s",filePath.str().c_str());
            if (access(filePath.str().c_str(),F_OK)==-1) {
              allTilesDownloaded=false;

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
                MapPosition pos;
                pos.setFromMercatorTileXY(zServer,x,y);
                lockAccess(__FILE__,__LINE__);
                fetchMapTile(pos,zMap);
                unlockAccess();
              }

            }

            // Shall we stop?
            if (quitProcessDownloadJobsThread)
              goto cleanup;
          }
        }
      }

nextJob:

      // Remember size of this job
      estimatedTotalStorageSpace+=estimatedJobStorageSpace;

      // Update app if this is an estimate job
      if (estimateOnly) {
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
    }
  }
  //DEBUG("estimatedStorageSpace: %lf MB",estimatedTotalStorageSpace);

cleanup:

  // Remove download job if all tiles have been downloaded
  for (std::list<std::string>::iterator i=finishedDownloadJobs.begin();i!=finishedDownloadJobs.end();i++) {
    std::string configPath="Map/DownloadJob[@name='" + *i + "']";
    c->removePath(configPath);
    processedDownloadJobs.remove(*i);
  }

  core->getThread()->unlockMutex(downloadJobsMutex);
}

} /* namespace GEODISCOVERER */
