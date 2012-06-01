//============================================================================
// Name        : MapSourceCalibratedPictures.cpp
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

MapSourceCalibratedPictures::MapSourceCalibratedPictures()  : MapSource() {
  type=MapSourceTypeCalibratedPictures;
  cacheData=NULL;
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

// Initializes the source
bool MapSourceCalibratedPictures::init()
{
  std::string filepath, filename;
  DIR *dp=NULL;
  struct dirent *dirp;
  struct stat filestat;
  double latNorth=-std::numeric_limits<double>::max(), latSouth=+std::numeric_limits<double>::max();
  double lngWest=+std::numeric_limits<double>::max(), lngEast=-std::numeric_limits<double>::max();;
  MapContainer *mapContainer;
  bool result;
  std::list<std::string> mapFilebases;
  std::ofstream ofs;
  std::ifstream ifs;
  //FILE *in;
  struct stat mapFolderStat,mapCacheStat;
  bool cacheRetrieved;
  std::string title;
  std::string mapPath=getFolderPath();
  std::string cacheFilepath=mapPath+"/cache.bin";

  // Check if we can use the cache
  cacheRetrieved=false;
  if (((stat(mapPath.c_str(),&mapFolderStat))==0)&&(stat(cacheFilepath.c_str(),&mapCacheStat)==0)) {

    // Is the cache newer than the folder?
    bool isNewerOrSameAge=false;
    if (mapCacheStat.st_mtime>=mapFolderStat.st_mtime) {
      isNewerOrSameAge=true;
    }
    if (isNewerOrSameAge) {

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

  // Could the cache not be loaded?
  if (!cacheRetrieved) {

    // Create a new progress dialog if required
    std::string title="Collecting files of map " + std::string(folder);
    DialogKey dialog=core->getDialog()->createProgress(title,0);

    // Go through all calibration files in the map directory
    dp = opendir( mapPath.c_str() );
    if (dp == NULL){
      ERROR("can not open map directory <%s> for reading available maps",folder.c_str());
      result=false;
      core->getDialog()->closeProgress(dialog);
      goto cleanup;
    }
    while ((dirp = readdir( dp )))
    {
      filename = std::string(dirp->d_name);
      filepath = mapPath + "/" + dirp->d_name;

      // Quit loop if requested
      if (core->getQuitCore())
        break;

      // If the file is a directory (or is in some way invalid) we'll skip it
      if (stat( filepath.c_str(), &filestat )) continue;
      if (S_ISDIR( filestat.st_mode ))         continue;

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
    closedir(dp);

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
    centerPosition.setLatScale(std::numeric_limits<double>::max());
    centerPosition.setLngScale(std::numeric_limits<double>::max());

    // Go through all found maps
    Int progress=1;
    Int maxZoomLevel=std::numeric_limits<Int>::min();
    Int minZoomLevel=std::numeric_limits<Int>::max();
    DEBUG("mapContainers.size()=%d",mapContainers.size());
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
      std::string filepath,filename;
      for (Int i=0;supportedExtension!="";i++) {
        supportedExtension=MapContainer::getCalibrationFileExtension(i);
        if (supportedExtension!="") {
          filename=filebase + "." + supportedExtension;
          filepath=mapPath + "/" + filename;
          if (access(filepath.c_str(),F_OK)==0) {
            extension=supportedExtension;
            break;
          }
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
      if (!(mapContainer->readCalibrationFile(mapPath,filebase,extension))) {
        result=false;
        delete mapContainer;
        goto cleanup;
      }
      mapContainers.push_back(mapContainer);

      // Remember the map with the lowest scale
      if ((mapContainer->getLatScale()<centerPosition.getLatScale())&&(mapContainer->getLngScale()<centerPosition.getLngScale())) {
        centerPosition.setLatScale(mapContainer->getLatScale());
        centerPosition.setLngScale(mapContainer->getLngScale());
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
    centerPosition.setLng(lngWest+(lngEast-lngWest)/2);
    centerPosition.setLat(latSouth+(latNorth-latSouth)/2);

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
  }

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
  centerPosition.store(ofs);

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

  PROFILE_START;
  bool success=true;

  // Check if the class has changed
  Int size=sizeof(MapSourceCalibratedPictures);
#ifdef TARGET_LINUX
  if (size!=408) {
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
  PROFILE_ADD("sanity check");

  // Create a busy dialog
  core->getMapSource()->openProgress("Reading cache of map " + folder, *((Int*)&cacheData[cacheSize-sizeof(Int)]));

  // Read the fields
  char *objectDataTemp=(char*)&mapSource->centerPosition;
  int objectSizeTemp=0;
  if (MapPosition::retrieve(cacheData,cacheSize)==NULL) {
    success=false;
    goto cleanup;
  }
  PROFILE_ADD("position retrieve");

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
  PROFILE_ADD("map tile retrieve");

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
  PROFILE_ADD("search tree retrieve");

  // The progress value was already consumed
  cacheSize-=sizeof(Int);
  cacheData+=sizeof(Int);

  // Object is initialized
  mapSource->setIsInitialized(true);

  // Close the dialog
cleanup:
  core->getMapSource()->closeProgress();
  PROFILE_ADD("cleanup");

  PROFILE_END;

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
