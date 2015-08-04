//============================================================================
// Name        : MapSourceCalibratedPictures.cpp
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

MapSourceCalibratedPictures::MapSourceCalibratedPictures(std::list<std::string> mapArchivePaths)  : MapSource() {
  type=MapSourceTypeCalibratedPictures;
  cacheData=NULL;
  this->mapArchivePaths=mapArchivePaths;
}

MapSourceCalibratedPictures::~MapSourceCalibratedPictures() {
  deinit();
  if (cacheData) {
    free(cacheData);
  }
}

// Operators
MapSourceCalibratedPictures &MapSourceCalibratedPictures::operator=(const MapSourceCalibratedPictures &rhs)
{
  FATAL("this object can not be copied",NULL);
}

// Clear the source
void MapSourceCalibratedPictures::deinit()
{
  MapSource::deinit();
}

// Loads all calibrated pictures in the given directory
bool MapSourceCalibratedPictures::collectMapTiles(std::string directory, std::list<std::string> &mapFilebases)
{
  std::string filename;

  // Go through all archives
  core->getMapSource()->lockMapArchives(__FILE__, __LINE__);
  for (std::list<ZipArchive*>::iterator i=mapArchives.begin();i!=mapArchives.end();i++) {

    // Go through all entries in the archive
    for (Int j=0;j<(*i)->getEntryCount();j++) {

      // Quit loop if requested
      if (core->getQuitCore())
        break;

      // Get the file name of the entry
      filename = (*i)->getEntryFilename(j);

      // If the file is a legend, extract it
      if (filename=="legend.png") {
        (*i)->exportEntry(filename,getFolderPath() + "/legend.png");
      }

      // If the file is a info.gds file, merge it's info into the existing list
      if (filename=="info.gds") {
        DEBUG("info.gds found",NULL);
        std::string gdsFilename = getFolderPath() + "/infoFromGDA.gds";
        (*i)->exportEntry("info.gds",gdsFilename);
        resolveGDSInfo(gdsFilename);
        unlink(gdsFilename.c_str());
      }

      // If this file is not a calibration file, skip it
      Int pos=filename.find_last_of(".");
      std::string extension=filename.substr(pos+1);
      std::string filebase=filename.substr(0,pos);

      // Check for supported extensions
      if (!MapContainer::calibrationFileIsSupported(extension))
        continue;

      // Remember the basename of the file
      mapFilebases.push_back(filebase);
    }
  }
  unlockMapArchives();

  return true;
}

// Initializes the source
bool MapSourceCalibratedPictures::init()
{
  double latNorth=-std::numeric_limits<double>::max(), latSouth=+std::numeric_limits<double>::max();
  double lngWest=+std::numeric_limits<double>::max(), lngEast=-std::numeric_limits<double>::max();;
  MapContainer *mapContainer;
  bool result;
  std::list<std::string> mapFilebases;
  std::ofstream ofs;
  std::ifstream ifs;
  //FILE *in;
  struct stat mapFolderStat,mapCacheStat,mapArchiveStat;
  bool cacheRetrieved;
  std::string title;
  std::string mapPath=getFolderPath();
  std::string cacheFilepath=mapPath+"/cache.bin";
  ZipArchive *mapArchive;
  std::string mapArchiveDir;
  std::string mapArchiveFile;
  char *mapArchivePathCStr = NULL;
  Int progress;
  DialogKey dialog;

  // Check if we can use the cache
  cacheRetrieved=false;
  if (((stat(mapPath.c_str(),&mapFolderStat))==0)&&(stat(cacheFilepath.c_str(),&mapCacheStat)==0)) {

    // Is the cache newer than the folder?
    bool isOlder=false;
    if (mapCacheStat.st_mtime<mapFolderStat.st_mtime) {
      isOlder=true;
    }
    for(std::list<std::string>::iterator i=mapArchivePaths.begin();i!=mapArchivePaths.end();i++) {
      if (stat((*i).c_str(),&mapArchiveStat)==0) {
        if (mapCacheStat.st_mtime<mapArchiveStat.st_mtime) {
          isOlder=true;
        }
      }
    }
    if (!isOlder) {

      // Load the cache
      ifs.open(cacheFilepath.c_str(),std::ios::binary);
      if (ifs.fail()) {
      //if (!(in=fopen(cacheFilepath.c_str(),"r"))) {
        remove(cacheFilepath.c_str());
        WARNING("can not open <%s> for reading",cacheFilepath.c_str());
      } else {

        // Load the complete file into memory
        if (!(cacheData=(char*)malloc(mapCacheStat.st_size+1))) {
          FATAL("can not allocate memory for cache",NULL);
          return false;
        }
        ifs.read(cacheData,mapCacheStat.st_size);
        ifs.close();
        //fread(cache,mapCacheStat.st_size,1,in);
        //fclose(in);
        cacheData[mapCacheStat.st_size]=0; // to prevent that strings never end
        Int cacheSize=mapCacheStat.st_size;

        // Retrieve the objects
        char *cacheData2=cacheData;
        bool success = MapSourceCalibratedPictures::retrieve(this,cacheData2,cacheSize,folder);
        if ((cacheSize!=0)||(!success)) {
          remove(cacheFilepath.c_str());
          deinit();
          if (core->getQuitCore()) {
            DEBUG("cache retrieve aborted because core quit requested",NULL);
            result=false;
            goto cleanup;
          }
          WARNING("falling back to full map read because cache is corrupted",NULL);
        } else {
          cacheRetrieved=true;
        }
      }
    }
  }

  // Open the zip archive that contains the maps
  title="Reading tiles of map " + folder;
  dialog=core->getDialog()->createProgress(title,mapArchivePaths.size());
  progress=0;
  lockMapArchives(__FILE__, __LINE__);
  for(std::list<std::string>::iterator i=mapArchivePaths.begin();i!=mapArchivePaths.end();i++) {
    std::string mapArchivePath=*i;
    mapArchivePathCStr=strdup(mapArchivePath.c_str());
    mapArchiveDir=std::string(dirname(mapArchivePathCStr));
    strcpy(mapArchivePathCStr,mapArchivePath.c_str());
    mapArchiveFile=std::string(basename(mapArchivePathCStr));
    free(mapArchivePathCStr);
    if (!(mapArchive=new ZipArchive(mapArchiveDir,mapArchiveFile))) {
      FATAL("can not create zip archive object",NULL);
      result=false;
      goto cleanup;
    }
    if (!mapArchive->init()) {
      ERROR("can not open tiles.gda in map directory <%s>",folder.c_str());
      result=false;
      goto cleanup;
    }
    mapArchives.push_back(mapArchive);
    progress++;
    core->getDialog()->updateProgress(dialog,title,progress);
  }
  unlockMapArchives();
  core->getDialog()->closeProgress(dialog);

  // Could the cache not be loaded?
  if (!cacheRetrieved) {

    // Prevent that phone switches off
    core->getDefaultScreen()->setWakeLock(true, __FILE__, __LINE__, false);

    // Create a new progress dialog if required
    std::string title="Collecting files of map " + std::string(folder);
    DialogKey dialog=core->getDialog()->createProgress(title,0);

    // Go through all calibration files in the map directory
    if (!collectMapTiles(mapPath,mapFilebases)) {
      result=false;
      goto cleanup;
    }
    //DEBUG("mapFilebases.size()=%d",mapFilebases.size());

    // Remove duplicates
    mapFilebases.sort();
    mapFilebases.unique();

    // Quit loop if requested
    core->getDialog()->closeProgress(dialog);
    if (core->getQuitCore()) {
      DEBUG("cache retrieve aborted because core quit requested",NULL);
      result=false;
      goto cleanup;
    }

    // Create the progress dialog
    title="Reading files of map " + std::string(folder);
    dialog=core->getDialog()->createProgress(title,mapFilebases.size());

    // Init variables
    centerPosition=new MapPosition();
    if (!centerPosition) {
      FATAL("can not create map position object",NULL);
    }
    centerPosition->setLatScale(std::numeric_limits<double>::max());
    centerPosition->setLngScale(std::numeric_limits<double>::max());

    // Go through all found maps
    Int progress=1;
    Int maxZoomLevel=std::numeric_limits<Int>::min();
    Int minZoomLevel=std::numeric_limits<Int>::max();
    //DEBUG("mapContainers.size()=%d",mapContainers.size());
    for (std::list<std::string>::const_iterator i=mapFilebases.begin();i!=mapFilebases.end();i++) {

      std::string filebase=*i;
      std::string extension;

      // Shall we stop?
      if (core->getQuitCore()) {
        DEBUG("cache retrieve aborted because core quit requested",NULL);
        result=false;
        goto cleanup;
      }

      // Check which calibration file is present
      std::string supportedExtension="-";
      std::string filepath;
      for (Int i=0;supportedExtension!="";i++) {
        supportedExtension=MapContainer::getCalibrationFileExtension(i);
        if (supportedExtension!="") {
          filepath=filebase + "." + supportedExtension;
          extension=supportedExtension;
          break;
        }
      }

      // Output some info
      Int percentage=round(((double)progress*100)/(double)mapFilebases.size());
      //INFO("reading map <%s> (%d%%)", filepath.c_str(), percentage);
      core->getDialog()->updateProgress(dialog,title,progress);
      progress++;

      // Create a new map container and read the calibration in
      mapContainer=new MapContainer();
      if (!mapContainer) {
        FATAL("can not create map container",NULL);
        result=false;
        goto cleanup;
      }
      if (!(mapContainer->readCalibrationFile(std::string(dirname((char*)filebase.c_str())),std::string(basename((char*)filebase.c_str())),extension))) {
        result=false;
        delete mapContainer;
        goto cleanup;
      }
      mapContainers.push_back(mapContainer);

      // Remember the map with the lowest scale
      if ((mapContainer->getLatScale()<centerPosition->getLatScale())&&(mapContainer->getLngScale()<centerPosition->getLngScale())) {
        centerPosition->setLatScale(mapContainer->getLatScale());
        centerPosition->setLngScale(mapContainer->getLngScale());
      }

      // Remember the minimum and maximum zoom levels
      if (mapContainer->getZoomLevel()<minZoomLevel)
        minZoomLevel=mapContainer->getZoomLevel();
      if (mapContainer->getZoomLevel()>maxZoomLevel)
        maxZoomLevel=mapContainer->getZoomLevel();

      // Remember the largest border
      if (mapContainer->getLatNorth()>latNorth)
        latNorth=mapContainer->getLatNorth();
      if (mapContainer->getLatSouth()<latSouth)
        latSouth=mapContainer->getLatSouth();
      if (mapContainer->getLngEast()>lngEast)
        lngEast=mapContainer->getLngEast();
      if (mapContainer->getLngWest()<lngWest)
        lngWest=mapContainer->getLngWest();
    }

    // Set the center position
    centerPosition->setLng(lngWest+(lngEast-lngWest)/2);
    centerPosition->setLat(latSouth+(latNorth-latSouth)/2);

    // Set the default map layer names
    for (Int z=maxZoomLevel;z>=minZoomLevel;z--) {
      std::stringstream s;
      s << z;
      mapLayerNameMap[s.str()]=z-minZoomLevel+1;
    }

    // Remember the original zoom levels
    this->minZoomLevel=minZoomLevel;
    this->maxZoomLevel=maxZoomLevel;
    DEBUG("minZoomLevel=%d maxZoomLevel=%d",minZoomLevel,maxZoomLevel);

    // Rename the layers
    renameLayers();

    // Normalize the zoom levels
    maxZoomLevel-=minZoomLevel;
    for(std::vector<MapContainer*>::iterator i=mapContainers.begin();i!=mapContainers.end();i++) {
      MapContainer *c=*i;
      c->setZoomLevel(c->getZoomLevel()-minZoomLevel+1);
    }
    minZoomLevel=0;

    // Init the zoom levels
    for (int z=0;z<=maxZoomLevel+1;z++) {
      zoomLevelSearchTrees.push_back(NULL);
    }

    // Update the search structures
    core->getDialog()->closeProgress(dialog);
    createSearchDataStructures(true);

    // Zoom levels with zero tiles are not supported
    for (int z=0;z<=maxZoomLevel+1;z++) {
      if (zoomLevelSearchTrees[z]==NULL) {
        ERROR("map <%s> can not be used because it has zoom levels with no tiles",std::string(folder).c_str());
        goto cleanup;
      }
    }

    // Store the map source contents for fast retrieval later
    title="Writing cache for map " + std::string(folder);
    dialog=core->getDialog()->createProgress(title,0);
    ofs.open(cacheFilepath.c_str(),std::ios::binary);
    if (ofs.fail()) {
      WARNING("can not open <%s> for writing",cacheFilepath.c_str());
      remove(cacheFilepath.c_str());
    } else {
      store(&ofs);
      if (ofs.bad()) {
        WARNING("can not store object into <%s>",cacheFilepath.c_str());
        remove(cacheFilepath.c_str());
      }
      ofs.close();
    }

    // Close progress
    core->getDialog()->closeProgress(dialog);

    // Extract legend if any
    if (mapArchive->getEntrySize("legend.png")!=0) {
      title="Extracting legend of map " + std::string(folder);
      dialog=core->getDialog()->createProgress(title,0);
      mapArchive->exportEntry("legend.png",getLegendPath());
      core->getDialog()->closeProgress(dialog);
    }

    // Reset wakelock
    core->getDefaultScreen()->setWakeLock(core->getConfigStore()->getIntValue("General","wakeLock",__FILE__, __LINE__),__FILE__, __LINE__,false);
  }

  //DEBUG("centerPosition.lng=%e centerPosition.lat=%e",centerPosition->getLng(),centerPosition->getLat());

  result=true;
cleanup:
  isInitialized=true;
  return result;
}

// Stores the contents of the search tree in a binary file
void MapSourceCalibratedPictures::storeSearchTree(std::ofstream *ofs, MapContainerTreeNode *node) {

  // Write the node index
  bool found=false;
  for (int i=0;i<mapContainers.size();i++) {
    if (node->getContents()==mapContainers[i]) {
      Storage::storeInt(ofs,i);
      found=true;
      break;
    }
  }
  if (!found) {
    FATAL("could not find tree node for given map container",NULL);
    return;
  }

  // Store the left node
  if (node->getLeftChild()==NULL) {
    Storage::storeBool(ofs,false);
  } else {
    Storage::storeBool(ofs,true);
    storeSearchTree(ofs,node->getLeftChild());
  }

  // Store the right node
  if (node->getRightChild()==NULL) {
    Storage::storeBool(ofs,false);
  } else {
    Storage::storeBool(ofs,true);
    storeSearchTree(ofs,node->getRightChild());
  }

}

// Store the contents of the object in a binary file
void MapSourceCalibratedPictures::store(std::ofstream *ofs) {

  Int totalTileCount=0;

  // Write the size of the object for detecting changes later
  Int size=sizeof(*this);
  Storage::storeInt(ofs,size);

  // Store all relevant fields
  centerPosition->store(ofs);
  DEBUG("minZoomLevel=%d maxZoomLevel=%d",minZoomLevel,maxZoomLevel);
  Storage::storeInt(ofs,minZoomLevel);
  Storage::storeInt(ofs,maxZoomLevel);

  // Store all container objects
  Storage::storeInt(ofs,mapContainers.size());
  for (int i=0;i<mapContainers.size();i++) {
    mapContainers[i]->store(ofs);
    totalTileCount+=mapContainers[i]->getTileCount();
  }

  // Store the search trees
  Storage::storeInt(ofs,zoomLevelSearchTrees.size());
  for (int i=0;i<zoomLevelSearchTrees.size();i++) {
    MapContainerTreeNode *startNode=zoomLevelSearchTrees[i];
    if (startNode) {
      Storage::storeBool(ofs,true);
      storeSearchTree(ofs,startNode);
    } else {
      Storage::storeBool(ofs,false);
    }
  }

  // Store the map layer names
  Storage::storeInt(ofs,mapLayerNameMap.size());
  for (MapLayerNameMap::iterator i=mapLayerNameMap.begin();i!=mapLayerNameMap.end();i++) {
    Storage::storeString(ofs,i->first);
    Storage::storeInt(ofs,i->second);
  }

  // Store the size for the progress dialog
  Int progressValueMax=totalTileCount+zoomLevelSearchTrees.size();
  Storage::storeInt(ofs,progressValueMax);
  //DEBUG("progressValueMax=%d",progressValueMax);
}

// Reads the contents of the search tree from a binary file
MapContainerTreeNode *MapSourceCalibratedPictures::retrieveSearchTree(MapSourceCalibratedPictures *mapSource, char *&cacheData, Int &cacheSize) {

  // Create a new map container tree node object
  MapContainerTreeNode *mapContainerTreeNode=new MapContainerTreeNode();
  if (!mapContainerTreeNode) {
    DEBUG("can not create map container tree node object",NULL);
    return NULL;
  }

  // Read the current node index and update the contents of the tree node
  Int index;
  Storage::retrieveInt(cacheData,cacheSize,index);
  if (index>=mapSource->mapContainers.size()) {
    DEBUG("node index is out of range, aborting retrieve",NULL);
    return NULL;
  }
  mapContainerTreeNode->setContents(mapSource->mapContainers[index]);

  // Read the left node
  bool hasLeftNode;
  Storage::retrieveBool(cacheData,cacheSize,hasLeftNode);
  if (hasLeftNode) {
    mapContainerTreeNode->setLeftChild(retrieveSearchTree(mapSource,cacheData,cacheSize));
  }

  // Read the right node
  bool hasRightNode;
  Storage::retrieveBool(cacheData,cacheSize,hasRightNode);
  if (hasRightNode) {
    mapContainerTreeNode->setRightChild(retrieveSearchTree(mapSource,cacheData,cacheSize));
  }

  // Return the node
  return mapContainerTreeNode;
}

// Reads the contents of the object from a binary file
bool MapSourceCalibratedPictures::retrieve(MapSourceCalibratedPictures *mapSource, char *&cacheData, Int &cacheSize, std::string folder) {

  //PROFILE_START;
  bool success=true;

  // Check if the class has changed
  Int size=sizeof(MapSourceCalibratedPictures);
#ifdef TARGET_LINUX
  if (size!=360) {
    FATAL("unknown size of object (%d), please adapt class storage",size);
    return false;
  }
#endif

  // Read the size of the object and check with current size
  size=0;
  Storage::retrieveInt(cacheData,cacheSize,size);
  if (size!=sizeof(MapSourceCalibratedPictures)) {
    DEBUG("stored size of object does not match implemented object size, aborting retrieve",NULL);
    return false;
  }
  //PROFILE_ADD("sanity check");

  // Create a busy dialog
  core->getMapSource()->openProgress("Reading cache of map " + folder, *((Int*)&cacheData[cacheSize-sizeof(Int)]));

  // Read the fields
  if ((mapSource->centerPosition=MapPosition::retrieve(cacheData,cacheSize))==NULL) {
    success=false;
    goto cleanup;
  }
  Storage::retrieveInt(cacheData,cacheSize,mapSource->minZoomLevel);
  Storage::retrieveInt(cacheData,cacheSize,mapSource->maxZoomLevel);
  DEBUG("minZoomLevel=%d maxZoomLevel=%d",mapSource->minZoomLevel,mapSource->maxZoomLevel);
  //PROFILE_ADD("position retrieve");

  // Read the map containers
  Storage::retrieveInt(cacheData,cacheSize,size);
  mapSource->mapContainers.resize(size);
  for (int i=0;i<size;i++) {

    // Retrieve the map container
    MapContainer *c=MapContainer::retrieve(cacheData,cacheSize);
    if (c==NULL) {
      mapSource->mapContainers.resize(i);
      success=false;
      goto cleanup;
    }
    mapSource->mapContainers[i]=c;

  }
  //PROFILE_ADD("map tile retrieve");

  // Read the search trees
  Storage::retrieveInt(cacheData,cacheSize,size);
  mapSource->zoomLevelSearchTrees.resize(size);
  for (int i=0;i<size;i++) {

    // Retrieve the search tree
    bool hasSearchTree;
    Storage::retrieveBool(cacheData,cacheSize,hasSearchTree);
    if (hasSearchTree) {
      MapContainerTreeNode *n=retrieveSearchTree(mapSource,cacheData,cacheSize);
      if (n==NULL) {
        mapSource->zoomLevelSearchTrees.resize(i);
        success=false;
        goto cleanup;
      }
      mapSource->zoomLevelSearchTrees[i]=n;
    }

    // Update the progress
    if (!core->getMapSource()->increaseProgress()) {
      mapSource->zoomLevelSearchTrees.resize(i+1);
      success=false;
      goto cleanup;
    }

  }
  //PROFILE_ADD("search tree retrieve");

  // Read the map layer names
  Storage::retrieveInt(cacheData,cacheSize,size);
  for (Int i=0;i<size;i++) {
    char *name;
    Int zoomLevel;
    Storage::retrieveString(cacheData,cacheSize,&name);
    Storage::retrieveInt(cacheData,cacheSize,zoomLevel);
    mapSource->mapLayerNameMap[name]=zoomLevel;
  }

  // The progress value was already consumed
  cacheSize-=sizeof(Int);
  cacheData+=sizeof(Int);

  // Object is initialized
  mapSource->setIsInitialized(true);

  // Close the dialog
cleanup:
  core->getMapSource()->closeProgress();
  //PROFILE_ADD("cleanup");

  //PROFILE_END;

  // Return result
  return success;
}

// Finds the calibrator for the given position
MapCalibrator *MapSourceCalibratedPictures::findMapCalibrator(Int zoomLevel, MapPosition pos, bool &deleteCalibrator) {
  deleteCalibrator=false;
  MapTile *tile=findMapTileByGeographicCoordinate(pos,zoomLevel,true,NULL);
  if (tile)
    return tile->getParentMapContainer()->getMapCalibrator();
  else
    return NULL;
}

// Returns the scale values for the given zoom level
void MapSourceCalibratedPictures::getScales(Int zoomLevel, double &latScale, double &lngScale) {
  MapContainer *c=zoomLevelSearchTrees[zoomLevel]->getContents();
  lngScale=c->getLngScale();
  latScale=c->getLatScale();
}


}
