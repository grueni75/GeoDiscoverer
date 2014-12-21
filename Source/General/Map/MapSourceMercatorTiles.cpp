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
//============================================================================

#include <Core.h>

namespace GEODISCOVERER {

// Constant values
const double MapSourceMercatorTiles::latBound = 85.0511287798;
const double MapSourceMercatorTiles::lngBound = 180.0;

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
}

// Destructor
MapSourceMercatorTiles::~MapSourceMercatorTiles() {

  // Stop the map downloader object
  delete mapDownloader;
  mapDownloader=NULL;

  // Free everything else
  deinit();
  core->getThread()->destroyMutex(accessMutex);
}

// Deinitializes the map source
void MapSourceMercatorTiles::deinit() {
  MapSource::deinit();
  if (mapDownloader) mapDownloader->deinit();
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

    // Add this archive if it is valid
    if (sscanf(dp->d_name,"tiles%d.gda",&nr)==1) {
      ZipArchive *archive = new ZipArchive(mapPath,dp->d_name);
      if ((archive)&&(archive->init())) {
        mapArchives.push_back(archive);
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
  centerPosition->setLatScale(((double)mapTileLength*pow(2.0,(double)minZoomLevel))/2.0/MapSourceMercatorTiles::latBound);
  centerPosition->setLngScale(((double)mapTileLength*pow(2.0,(double)minZoomLevel))/2.0/MapSourceMercatorTiles::lngBound);

  // Init the zoom levels
  for (int z=0;z<(maxZoomLevel-minZoomLevel)+2;z++) {
    zoomLevelSearchTrees.push_back(NULL);
  }

  // Finished
  isInitialized=true;
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
  std::stringstream imageFileFolder; imageFileFolder << z << "/" << x;
  std::stringstream imageFileBase; imageFileBase << z << "_" << x << "_" << y;
  std::string imageFileExtension = "png";
  ImageType imageType = ImageTypePNG;
  std::string imageFileName = imageFileBase.str() + "." + imageFileExtension;
  std::string imageFilePath = imageFileFolder.str() + "/" + imageFileName;
  std::string calibrationFileName = imageFileBase.str() + ".gdm";
  std::string calibrationFilePath = imageFileFolder.str() + "/" + calibrationFileName;

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
  lockMapArchives(__FILE__, __LINE__);
  bool found=false;
  for(std::list<ZipArchive*>::iterator i=mapArchives.begin();i!=mapArchives.end();i++) {
    ZipArchive *mapArchive = *i;
    if (mapArchive->getEntrySize(imageFilePath)>0) {
      found=true;
      break;
    }
  }
  unlockMapArchives();
  if (!found) {
    mapDownloader->queueMapContainerDownload(mapContainer);
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
    MapPosition pos=area.getCenterPos();
    if (preferredNeigbor) {
      preferredNeigbor->getNeighborPos(area,pos);
    }
    return fetchMapTile(pos,area.getZoomLevel());

  } else {
    return result;
  }
}

// Performs maintenance (e.g., recreate degraded search tree)
void MapSourceMercatorTiles::maintenance() {

  // Was the source modified?
  if (contentsChanged) {

    // Remove map containers from list until cache size is reached again
    std::list<MapContainer*> obsoleteMapContainers;
    lockAccess(__FILE__, __LINE__);
    for(std::vector<MapContainer*>::iterator i=mapContainers.begin();i!=mapContainers.end();) {
      if (mapContainers.size()>mapContainerCacheSize) {

        // Remove container if it is not currently downloaded and if it is not visible
        MapContainer *c=*i;
        if ((!c->isDrawn())&&(c->getDownloadComplete()&&(!c->getOverlayGraphicInvalid()))) {
          obsoleteMapContainers.push_back(c);
          i=mapContainers.erase(i);
        } else {
          i++;
        }

      } else {
        break;
      }
    }

    // Recreate the search tree
    createSearchDataStructures();
    unlockAccess();

    // Delete the map containers
    for(std::list<MapContainer*>::iterator i=obsoleteMapContainers.begin();i!=obsoleteMapContainers.end();i++) {
      MapContainer *c=*i;
      //DEBUG("deleting map container 0x%08x",c);
      core->getMapCache()->removeTile(c->getMapTiles()->front());
      core->getNavigationEngine()->removeGraphics(c);
      delete c;
    }

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

// Reads information about the map
bool MapSourceMercatorTiles::parseGDSInfo()
{
  bool result=false;
  bool tileServerFound=false;
  bool minZoomLevelFound,maxZoomLevelFound;
  std::stringstream in;
  std::string infoFilePath = getFolderPath() + "/info.gds";

  // Loop over the elements and extract the information
  tileServerFound=false;
  minZoomLevelFound=false;
  maxZoomLevelFound=false;
  std::string serverURL;
  bool serverURLFound=false;
  double overlayAlpha;
  bool overlayAlphaFound=false;
  for (std::list<std::vector<std::string> >::iterator i=gdsElements.begin();i!=gdsElements.end();i++) {
    std::vector<std::string> element = *i;
    if (element[0]=="TileServer") {
      if (element[1]=="serverURL") {
        serverURL=element[2];
        serverURLFound=true;
      }
      if (element[1]=="overlayAlpha") {
        in.str(element[2]);
        in.clear();
        in >> overlayAlpha;
        overlayAlphaFound=true;
      }
    }
    if (element[0]=="/TileServer") {
      if (!serverURLFound) {
        ERROR("one TileServer element has no serverURL element in <%s>",infoFilePath.c_str());
        goto cleanup;
      }
      if (!overlayAlphaFound) {
        ERROR("one TileServer element has no overlayAlpha element in <%s>",infoFilePath.c_str());
        goto cleanup;
      }
      mapDownloader->addTileServer(serverURL,overlayAlpha);
      tileServerFound=true;
      serverURLFound=false;
      overlayAlphaFound=false;
    }
    if (element[0]=="minZoomLevel") {
      minZoomLevelFound=true;
      in.str(element[1]);
      in.clear();
      in >> minZoomLevel;
    }
    if (element[0]=="maxZoomLevel") {
      maxZoomLevelFound=true;
      in.str(element[1]);
      in.clear();
      in >> maxZoomLevel;
    }
  }
  if (!tileServerFound) {
    ERROR("no tileServer element found in <%s>",infoFilePath.c_str());
    goto cleanup;
  }
  if (!minZoomLevelFound) {
    ERROR("minZoomLevel not found in <%s>",infoFilePath.c_str());
    goto cleanup;
  }
  if (!maxZoomLevelFound) {
    ERROR("maxZoomLevel not found in <%s>",infoFilePath.c_str());
    goto cleanup;
  }
  result=true;

cleanup:

  return result;
}

} /* namespace GEODISCOVERER */
