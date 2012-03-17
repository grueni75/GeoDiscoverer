//============================================================================
// Name        : MapSource.cpp
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

MapSource::MapSource(bool isScratchOnly, bool doNotDelete) {
  folder=core->getConfigStore()->getStringValue("Map","folder","Folder that contains calibrated maps","Default");
  neighborDegreeTolerance=core->getConfigStore()->getDoubleValue("Map","neighborDegreeTolerance","// Maximum allowed difference in degrees to classify a tile as a neighbor",1e-7);
  mapTileLength=core->getConfigStore()->getIntValue("Map","tileLength","Width and height of a tile.",256);
  isInitialized=false;
  this->isScratchOnly=isScratchOnly;
  this->doNotDelete=doNotDelete;
  objectData=NULL;
}

MapSource::~MapSource() {
  if (!isScratchOnly) {
    deinit();
    if (objectData)
        free(objectData);
  }
}

// Clear the source
void MapSource::deinit()
{
  if (isInitialized) {
    for (std::vector<MapContainer*>::const_iterator i=mapContainers.begin();i!=mapContainers.end();i++) {
      MapContainer::destruct(*i);
    }
    mapContainers.clear();
    centerPosition=MapPosition();
    isInitialized=false;
    for(std::vector<MapContainerTreeNode*>::iterator i=zoomLevelSearchTrees.begin();i!=zoomLevelSearchTrees.end();i++) {
      MapContainerTreeNode *t=*i;
      MapContainerTreeNode::destruct(t);
    }
    zoomLevelSearchTrees.clear();
  }
}

// Initializes the source
bool MapSource::init()
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
  std::string path = core->getHomePath() + "/Map/" + folder;
  std::string cacheFilepath=path+"/cache.bin";
  struct stat mapFolderStat,mapCacheStat;
  bool cacheRetrieved;
  std::string title;

  // Check if we can use the cache
  cacheRetrieved=false;
  if (((stat(path.c_str(),&mapFolderStat))==0)&&(stat(cacheFilepath.c_str(),&mapCacheStat)==0)) {

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
        char *cacheData;
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

        // Reserve memory for the required objects
        Int objectSize=*((Int*)&cacheData[cacheSize-sizeof(Int)]);
        cacheSize-=sizeof(Int);
        if (!(objectData=(char*)malloc(objectSize))) {
          FATAL("can not allocate memory for objects",NULL);
          return false;
        }

        // Retrieve the objects
        char *objectData2=objectData;
        MapSource *newMapSource=MapSource::retrieve(cacheData,cacheSize,objectData2,objectSize,folder);
        if ((cacheSize!=0)||(objectSize!=0)||(newMapSource==NULL)) {
          remove(cacheFilepath.c_str());
          if (newMapSource!=NULL) {
            newMapSource->objectData=NULL;
            newMapSource->isScratchOnly=false;
            MapSource::destruct(newMapSource);
          }
          free(objectData);
          objectData=NULL;
          WARNING("falling back to full map read because cache is corrupted",NULL);
        } else {
          *this=*newMapSource;
          this->isScratchOnly=false;
          cacheRetrieved=true;
        }
      }
    }
  }

  // Could the cache not be loaded?
  if (!cacheRetrieved) {

    // Create a new progress dialog if required
    std::string title="Collecting files of map " + folder;
    DialogKey dialog=core->getDialog()->createProgress(title,0);

    // Go through all calibration files in the map directory
    dp = opendir( path.c_str() );
    if (dp == NULL){
      ERROR("can not open map directory <%s> for reading available maps",folder.c_str());
      return false;
    }
    while ((dirp = readdir( dp )))
    {
      filename = std::string(dirp->d_name);
      filepath = path + "/" + dirp->d_name;

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

    // Remove duplicates
    mapFilebases.sort();
    mapFilebases.unique();

    // Create the progress dialog
    core->getDialog()->closeProgress(dialog);
    title="Reading files of map " + folder;
    dialog=core->getDialog()->createProgress(title,mapFilebases.size());

    // Init variables
    centerPosition.setLatScale(std::numeric_limits<double>::max());
    centerPosition.setLngScale(std::numeric_limits<double>::max());

    // Go through all found maps
    Int progress=1;
    Int maxZoomLevel=std::numeric_limits<Int>::min();
    Int minZoomLevel=std::numeric_limits<Int>::max();
    for (std::list<std::string>::const_iterator i=mapFilebases.begin();i!=mapFilebases.end();i++) {

      std::string filebase=*i;
      std::string extension;

      // Check which calibration file is present
      std::string supportedExtension="-";
      std::string filepath,filename;
      for (Int i=0;supportedExtension!="";i++) {
        supportedExtension=MapContainer::getCalibrationFileExtension(i);
        if (supportedExtension!="") {
          filename=filebase + "." + supportedExtension;
          filepath=path + "/" + filename;
          FILE *in;
          in=fopen(filepath.c_str(),"r");
          if (in) {
            extension=supportedExtension;
            fclose(in);
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
      if (!(mapContainer->readCalibrationFile(path,filebase,extension))) {
        result=false;
        delete mapContainer;
        goto cleanup;
      }
      mapContainers.push_back(mapContainer);

      // Add the new container to the sorted vectors for search tree creation
      Int newMapIndex=mapContainers.size()-1;
      insertMapContainerToSortedList(&mapsIndexByLatNorth,mapContainer,newMapIndex,GeographicBorderLatNorth);
      insertMapContainerToSortedList(&mapsIndexByLatSouth,mapContainer,newMapIndex,GeographicBorderLatSouth);
      insertMapContainerToSortedList(&mapsIndexByLngWest,mapContainer,newMapIndex,GeographicBorderLngWest);
      insertMapContainerToSortedList(&mapsIndexByLngEast,mapContainer,newMapIndex,GeographicBorderLngEast);

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

    // Normalize the zoom levels and create index list for each zoom level
    maxZoomLevel-=minZoomLevel;
    maxZoomLevel++;
    std::vector< std::vector<Int> > mapsIndexByZoomLevel;
    for (int i=0;i<=maxZoomLevel;i++) {
      mapsIndexByZoomLevel.push_back(std::vector<int>());
    }
    for(std::vector<Int>::iterator i=mapsIndexByLatNorth.begin();i!=mapsIndexByLatNorth.end();i++) {
      Int index=*i;
      MapContainer *c=mapContainers[index];
      c->setZoomLevel(c->getZoomLevel()-minZoomLevel+1);
      mapsIndexByZoomLevel[c->getZoomLevel()].push_back(index);
      mapsIndexByZoomLevel[0].push_back(index);  // zoom level 0 contains all containers and tiles
    }
    minZoomLevel=0;

    // Update the search structures
    core->getDialog()->closeProgress(dialog);
    title="Creating search tree for map " + folder;
    progress=1;
    dialog=core->getDialog()->createProgress(title,maxZoomLevel+1);
    for(Int i=0;i<=maxZoomLevel;i++) {
      core->getDialog()->updateProgress(dialog,title,progress);
      std::vector<Int> zoomLevelMapsIndex=mapsIndexByZoomLevel[i];
      zoomLevelSearchTrees.push_back(createSearchTree(NULL,false,GeographicBorderLatNorth,zoomLevelMapsIndex));
      progress++;
    }
    core->getDialog()->closeProgress(dialog);

    // Store the map source contents for fast retrieval later
    title="Writing cache for map " + folder;
    dialog=core->getDialog()->createProgress(title,0);
    ofs.open(cacheFilepath.c_str(),std::ios::binary);
    if (ofs.fail()) {
      WARNING("can not open <%s> for writing",cacheFilepath.c_str());
      remove(cacheFilepath.c_str());
    } else {
      Int memorySize=0;
      store(&ofs,memorySize);
      Storage::storeInt(&ofs,memorySize);  // for computing the required memory
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
  if (dp) closedir(dp);
  isInitialized=true;
  return result;
}

// Sorts the list associated with the given border
void MapSource::insertMapContainerToSortedList(std::vector<Int> *list, MapContainer *newMapContainer, Int newMapContainerIndex, GeographicBorder border) {
  std::vector<Int>::iterator i;
  bool inserted=false;
  for (i=list->begin();i<list->end();i++) {
    if (mapContainers[*i]->getBorder(border) > newMapContainer->getBorder(border)) {
      list->insert(i,newMapContainerIndex);
      inserted=true;
      break;
    }
  }
  if (!inserted)
    list->push_back(newMapContainerIndex);
}

// Returns the map tile in which the position lies
MapTile *MapSource::findMapTileByGeographicCoordinates(MapPosition pos, Int zoomLevel, bool lockZoomLevel, MapContainer *preferredMapContainer) {

  bool betterMapContainerFound=false;
  MapPosition bestPos;
  double distToNearestLngScale=-1, distToNearestLatScale=-1;

  // Get the search tree to use
  MapContainerTreeNode *startNode=zoomLevelSearchTrees[zoomLevel];

  // First find the map container matching the given position
  MapContainer *foundMapContainer=findMapContainerByGeographicCoordinates(pos,preferredMapContainer,startNode,GeographicBorderLatNorth,bestPos,distToNearestLngScale,distToNearestLatScale,betterMapContainerFound);
  //DEBUG("found map container = %08x",foundMapContainer);

  // If no container can be found, try to fall back to zoom level 0 (all tiles)
  if ((!foundMapContainer)&&(zoomLevel!=0)&&(!lockZoomLevel)) {
    startNode=zoomLevelSearchTrees[0];
    foundMapContainer=findMapContainerByGeographicCoordinates(pos,preferredMapContainer,startNode,GeographicBorderLatNorth,bestPos,distToNearestLngScale,distToNearestLatScale,betterMapContainerFound);
  }

  // Now search the closest tile in the map
  if (foundMapContainer) {
    return foundMapContainer->findMapTileByPictureCoordinates(bestPos);
  } else {
    return NULL;
  }
}

// Returns the map tile in which the position lies
MapContainer *MapSource::findMapContainerByGeographicCoordinates(MapPosition pos, MapContainer *preferredMapContainer, MapContainerTreeNode* currentMapContainerTreeNode, GeographicBorder currentDimension, MapPosition &bestPos, double &distToNearestLngScale, double &distToNearestLatScale, bool &betterMapContainerFound) {

  MapContainer *bestMapContainer=NULL,*bestMapContainerLeft=NULL,*bestMapContainerRight=NULL;
  bool betterMapContainerFoundRight=NULL,betterMapContainerFoundLeft=NULL;
  MapPosition bestPosRight, bestPosLeft;
  double distToNearestLngScaleRight, distToNearestLatScaleRight;
  double distToNearestLngScaleLeft, distToNearestLatScaleLeft;
  bool useLeftChild=true,useRightChild=true;
  GeographicBorder nextDimension;

  // Set variables
  betterMapContainerFound=false;

  // Abort if map is null
  if (!currentMapContainerTreeNode)
    return NULL;
  MapContainer *currentMapContainer=currentMapContainerTreeNode->getContents();

  // Check that the point lies within the map
  if (  (currentMapContainer->getLatNorth()>=pos.getLat())&&(currentMapContainer->getLatSouth()<=pos.getLat())
      &&(currentMapContainer->getLngEast() >=pos.getLng())&&(currentMapContainer->getLngWest() <=pos.getLng())) {

    // Compute the position in this map
    bool overflowOccured=false;
    overflowOccured=!(currentMapContainer->getMapCalibrator()->setPictureCoordinates(pos));
    if (!overflowOccured) {

      // Check if the position lies in this map
      if ((pos.getX()>=0)&&(pos.getX()<currentMapContainer->getWidth())&&(pos.getY()>=0)&&(pos.getY()<currentMapContainer->getHeight())) {

        double distToLngScale=fabs(currentMapContainer->getLngScale()-pos.getLngScale());
        double distToLatScale=fabs(currentMapContainer->getLatScale()-pos.getLatScale());

        // Use the map that matches the scale closest if lockScale is true
        bool newCandidateFound=false;
        if (distToNearestLngScale==-1) {
          newCandidateFound=true;
        } else {
          if ((distToLngScale<distToNearestLngScale)&&(distToLatScale<distToNearestLatScale)) {
            newCandidateFound=true;
          }
        }
        if (newCandidateFound) {

          // Remember the candidate
          distToNearestLngScale=distToLngScale;
          distToNearestLatScale=distToLatScale;
          bestMapContainer=currentMapContainer;
          bestPos=pos;
          betterMapContainerFound=true;

          // Stop search if this map is the preferred one
          if (bestMapContainer==preferredMapContainer)
            return bestMapContainer;
        }
      }
    }
  }

  // Decide with which branches of the tree to continue
  switch(currentDimension) {
    case GeographicBorderLatNorth:
      nextDimension=GeographicBorderLatSouth;
      if (currentMapContainer->getLatNorth()<pos.getLat()) {
        useLeftChild=false;
      }
      break;
    case GeographicBorderLatSouth:
      nextDimension=GeographicBorderLngEast;
      if (currentMapContainer->getLatSouth()>pos.getLat()) {
        useRightChild=false;
      }
      break;
    case GeographicBorderLngEast:
      nextDimension=GeographicBorderLngWest;
      if (currentMapContainer->getLngEast()<pos.getLng()) {
        useLeftChild=false;
      }
      break;
    case GeographicBorderLngWest:
      nextDimension=GeographicBorderLatNorth;
      if (currentMapContainer->getLngWest()>pos.getLng()) {
        useRightChild=false;
      }
      break;
  }

  // Get the best tiles from the left and the right branch
  distToNearestLatScaleRight=distToNearestLatScale;
  distToNearestLngScaleRight=distToNearestLngScale;
  bestPosRight=bestPos;
  if (useRightChild) {
    bestMapContainerRight=findMapContainerByGeographicCoordinates(pos,preferredMapContainer,currentMapContainerTreeNode->getRightChild(),nextDimension,bestPosRight,distToNearestLngScaleRight,distToNearestLatScaleRight,betterMapContainerFoundRight);
  }
  distToNearestLatScaleLeft=distToNearestLatScaleRight;
  distToNearestLngScaleLeft=distToNearestLngScaleRight;
  bestPosLeft=bestPosRight;
  if ((useLeftChild)&&(!preferredMapContainer||bestMapContainerRight!=preferredMapContainer)) {
    bestMapContainerLeft=findMapContainerByGeographicCoordinates(pos,preferredMapContainer,currentMapContainerTreeNode->getLeftChild(),nextDimension,bestPosLeft,distToNearestLngScaleLeft,distToNearestLatScaleLeft,betterMapContainerFoundLeft);
  }

  // Decide on the result
  if (betterMapContainerFoundLeft) {
    distToNearestLatScale=distToNearestLatScaleLeft;
    distToNearestLngScale=distToNearestLngScaleLeft;
    bestPos=bestPosLeft;
    betterMapContainerFound=true;
    return bestMapContainerLeft;
  }
  if (betterMapContainerFoundRight) {
    distToNearestLatScale=distToNearestLatScaleRight;
    distToNearestLngScale=distToNearestLngScaleRight;
    bestPos=bestPosRight;
    betterMapContainerFound=true;
    return bestMapContainerRight;
  }
  return bestMapContainer;
}

// Returns the map tile that lies in a given area
MapTile *MapSource::findMapTileByGeographicArea(MapArea area, MapTile *preferredNeigbor, MapContainer* &usedMapContainer) {

  MapArea bestTranslatedArea;
  double bestDistance=std::numeric_limits<double>::max();
  bool betterMapContainerFound=false;

  // Get the search tree to use
  MapContainerTreeNode *startNode=zoomLevelSearchTrees[area.getZoomLevel()];

  // First find the map container matching the given area
  MapContainer *foundMapContainer=findMapContainerByGeographicArea(area,preferredNeigbor,startNode,GeographicBorderLatNorth,bestDistance,bestTranslatedArea,betterMapContainerFound);
  //DEBUG("found map container = %08x",foundMapContainer);

  // Now search the closest tile in the map
  usedMapContainer=foundMapContainer;
  if (foundMapContainer) {
    return foundMapContainer->findMapTileByPictureArea(bestTranslatedArea,preferredNeigbor);
  } else {
    return NULL;
  }
}

// Returns the map container that lies in a given area
MapContainer *MapSource::findMapContainerByGeographicArea(MapArea area, MapTile *preferredNeigbor, MapContainerTreeNode* currentMapContainerTreeNode, GeographicBorder currentDimension, double &bestDistance, MapArea &bestTranslatedArea, bool &betterMapContainerFound, std::list<MapContainer*> *foundMapContainers) {

  MapContainer *bestMapContainer=NULL,*bestMapContainerLeft=NULL,*bestMapContainerRight=NULL;
  bool betterMapContainerFoundRight=false, betterMapContainerFoundLeft=false;
  MapArea bestTranslatedAreaLeft,bestTranslatedAreaRight;
  GeographicBorder nextDimension;
  bool useRightChild=true,useLeftChild=true;
  double bestDistanceLeft,bestDistanceRight;

  // Set variables
  betterMapContainerFound=false;

  // Abort if map is null
  if (!currentMapContainerTreeNode)
    return NULL;
  MapContainer *currentMapContainer=currentMapContainerTreeNode->getContents();

  // Check that the map overlaps the area
  if (  (currentMapContainer->getLatNorth()>=area.getLatSouth())&&(currentMapContainer->getLatSouth()<=area.getLatNorth())
      &&(currentMapContainer->getLngEast() >=area.getLngWest()) &&(currentMapContainer->getLngWest() <=area.getLngEast())) {

    // Check that the scale fits
    bool candidateMatchesScale=false;
    if (area.getZoomLevel()==currentMapContainer->getZoomLevel()) {
      candidateMatchesScale=true;
    }
    if (candidateMatchesScale) {

      // Shall we return all matching containers?
      if (foundMapContainers) {

        foundMapContainers->push_back(currentMapContainer);

      } else {

        // Compute the boundaries of the map within the area
        bool overflowOccured=false;
        MapPosition pos=area.getRefPos();
        overflowOccured=!(currentMapContainer->getMapCalibrator()->setPictureCoordinates(pos));
        MapArea translatedArea=area;
        if (!overflowOccured) {
          translatedArea.setRefPos(pos);
          Int diff;
          diff=(area.getXEast()-area.getRefPos().getX());
          if (((pos.getX()>0)&&(std::numeric_limits<Int>::max()-pos.getX())<diff))
            overflowOccured=true;
          translatedArea.setXEast(pos.getX()+diff);
          diff=(area.getRefPos().getX()-area.getXWest());
          if ((pos.getX()<0)&&((pos.getX()-std::numeric_limits<Int>::min())<diff))
            overflowOccured=true;
          translatedArea.setXWest(pos.getX()-diff);
          diff=area.getYNorth()-area.getRefPos().getY();
          if ((pos.getY()<0)&&((pos.getY()-std::numeric_limits<Int>::min())<diff))
            overflowOccured=true;
          translatedArea.setYNorth(pos.getY()-diff);
          diff=area.getRefPos().getY()-area.getYSouth();
          if ((pos.getY()>0)&&((std::numeric_limits<Int>::max()-pos.getY())<diff))
            overflowOccured=true;
          translatedArea.setYSouth(pos.getY()+diff);
        }
        Int mapYNorth=0;
        Int mapYSouth=currentMapContainer->getHeight()-1;
        Int mapXWest=0;
        Int mapXEast=currentMapContainer->getWidth()-1;

        // No overflow occured?
        if (!overflowOccured) {

          // Check that the map overlaps the area
          //DEBUG("translated area: %d %d %d %d",translatedArea.getYNorth(),translatedArea.getXEast(),translatedArea.getYSouth(),translatedArea.getXWest());
          if ((mapXWest<translatedArea.getXEast())&&(mapXEast>translatedArea.getXWest())&&
              (mapYNorth<translatedArea.getYSouth())&&(mapYSouth>translatedArea.getYNorth())) {

            // Preferred neighbor given?
            bool stopSearch=false;
            bool containerFound=false;
            if (preferredNeigbor) {

              // Use the container directly if the preferred neighbor belongs to it
              if (currentMapContainer==preferredNeigbor->getParentMapContainer()) {
                containerFound=true;
                stopSearch=true;
                bestDistance=0;
              } else {

                // Compute the distance to the preferred neighbor
                double diffLat=preferredNeigbor->getLatCenter()-currentMapContainer->getLatCenter();
                double diffLng=preferredNeigbor->getLngCenter()-currentMapContainer->getLngCenter();
                double distance=diffLat*diffLat+diffLng*diffLng;

                //DEBUG("current map container %s with distance %e",currentMapContainer->getImageFileName().c_str(),distance);

                // Check if it is better than the one found so far
                if (distance<bestDistance) {
                  //DEBUG("better map container %s found with distance %e",currentMapContainer->getImageFileName().c_str(),distance);
                  bestDistance=distance;
                  containerFound=true;
                }

              }
            } else {
              containerFound=true;
              stopSearch=true;
              bestDistance=0;
            }

            // Remember the result if requested
            if (containerFound) {
              bestMapContainer=currentMapContainer;
              bestTranslatedArea=translatedArea;
              betterMapContainerFound=true;
            }

            // Stop the search if requested
            if (stopSearch) {
              return bestMapContainer;
            }

          }
        }
      }
    }
  }

  // Decide with which branches of the tree to continue
  switch(currentDimension) {
    case GeographicBorderLatNorth:
      nextDimension=GeographicBorderLatSouth;
      if (currentMapContainer->getLatNorth()<area.getLatSouth()) {
        useLeftChild=false;
      }
      break;
    case GeographicBorderLatSouth:
      nextDimension=GeographicBorderLngEast;
      if (currentMapContainer->getLatSouth()>area.getLatNorth()) {
        useRightChild=false;
      }
      break;
    case GeographicBorderLngEast:
      nextDimension=GeographicBorderLngWest;
      if (currentMapContainer->getLngEast()<area.getLngWest()) {
        useLeftChild=false;
      }
      break;
    case GeographicBorderLngWest:
      nextDimension=GeographicBorderLatNorth;
      if (currentMapContainer->getLngWest()>area.getLngEast()) {
        useRightChild=false;
      }
      break;
  }

  // Get the best tiles from the left and the right branch
  bestDistanceRight=bestDistance;
  if (useRightChild) {
    //DEBUG("search for better matching tile in right branch",NULL);
    bestMapContainerRight=findMapContainerByGeographicArea(area,preferredNeigbor,currentMapContainerTreeNode->getRightChild(),nextDimension,bestDistanceRight,bestTranslatedAreaRight,betterMapContainerFoundRight,foundMapContainers);
  }
  bestDistanceLeft=bestDistanceRight;
  if ((useLeftChild)&&((!preferredNeigbor&&!bestMapContainerRight)||(preferredNeigbor&&bestMapContainerRight!=preferredNeigbor->getParentMapContainer()))) {
    //DEBUG("search for better matching tile in left branch",NULL);
    bestMapContainerLeft=findMapContainerByGeographicArea(area,preferredNeigbor,currentMapContainerTreeNode->getLeftChild(),nextDimension,bestDistanceLeft,bestTranslatedAreaLeft,betterMapContainerFoundLeft,foundMapContainers);
  }
  //DEBUG("current best map container = 0x%08x",bestMapContainer);
  if (betterMapContainerFoundLeft) {
    bestTranslatedArea=bestTranslatedAreaLeft;
    bestDistance=bestDistanceLeft;
    betterMapContainerFound=true;
    return bestMapContainerLeft;
  }
  if (betterMapContainerFoundRight) {
    bestTranslatedArea=bestTranslatedAreaRight;
    bestDistance=bestDistanceRight;
    betterMapContainerFound=true;
    return bestMapContainerRight;
  }
  return bestMapContainer;
}

// Returns a list of map containers that overlap the given area
std::list<MapContainer*> MapSource::findMapContainersByGeographicArea(MapArea area) {

  MapArea bestTranslatedArea;
  double bestDistance=std::numeric_limits<double>::max();
  bool betterMapContainerFound=false;
  std::list<MapContainer*> result;

  // Get the search tree to use
  MapContainerTreeNode *startNode=zoomLevelSearchTrees[area.getZoomLevel()];

  // Find all map containers matching the given area
  findMapContainerByGeographicArea(area,NULL,startNode,GeographicBorderLatNorth,bestDistance,bestTranslatedArea,betterMapContainerFound,&result);
  return result;
}

// Creates a sorted index vector from a given sorted index vector masked by range on an other index vector
void MapSource::createMaskedIndexVector(std::vector<Int> *currentIndexVector, std::vector<Int> *allowedIndexVector, std::vector<Int> &sortedIndexVector) {
  sortedIndexVector.clear();
  for(Int i=0;i<currentIndexVector->size();i++) {
    Int t=(*currentIndexVector)[i];
    for(Int j=0;j<allowedIndexVector->size();j++) {
      if ((*allowedIndexVector)[j]==t) {
        sortedIndexVector.push_back(t);
        break;
      }
    }
  }
}

// Creates the search tree
MapContainerTreeNode *MapSource::createSearchTree(MapContainerTreeNode *parentNode, bool leftBranch, GeographicBorder dimension, std::vector<Int> remainingMapsIndex) {

  GeographicBorder nextDimension;
  Int medianIndex;
  MapContainer *rootNode,*tempNode;
  bool lessEqual=false;
  bool greaterEqual=false;
  std::vector<Int> sortedVector;
  MapContainerTreeNode *mapContainerTreeNode;

  // Update variables depending on the dimension
  switch(dimension) {
    case GeographicBorderLatNorth:
      nextDimension=GeographicBorderLatSouth;
      createMaskedIndexVector(&mapsIndexByLatNorth,&remainingMapsIndex,sortedVector);
      lessEqual=true;
      break;
    case GeographicBorderLatSouth:
      nextDimension=GeographicBorderLngEast;
      createMaskedIndexVector(&mapsIndexByLatSouth,&remainingMapsIndex,sortedVector);
      greaterEqual=true;
      break;
    case GeographicBorderLngEast:
      nextDimension=GeographicBorderLngWest;
      createMaskedIndexVector(&mapsIndexByLngEast,&remainingMapsIndex,sortedVector);
      lessEqual=true;
      break;
    case GeographicBorderLngWest:
      nextDimension=GeographicBorderLatNorth;
      createMaskedIndexVector(&mapsIndexByLngWest,&remainingMapsIndex,sortedVector);
      greaterEqual=true;
      break;
  }

  // Compute median index
  medianIndex=sortedVector.size()/2;
  if (lessEqual) {
    while ((medianIndex<sortedVector.size()-1)&&(mapContainers[sortedVector[medianIndex]]->getBorder(dimension)==mapContainers[sortedVector[medianIndex+1]]->getBorder(dimension)))
      medianIndex++;
  }
  if (greaterEqual) {
    while ((medianIndex>0)&&(mapContainers[(sortedVector)[medianIndex]]->getBorder(dimension)==mapContainers[(sortedVector)[medianIndex-1]]->getBorder(dimension)))
      medianIndex--;
  }

  // Add node to the tree
  if (!(mapContainerTreeNode=new MapContainerTreeNode())) {
    FATAL("can not create map container tree object",NULL);
    return NULL;
  }
  rootNode=mapContainers[(sortedVector)[medianIndex]];
  mapContainerTreeNode->setContents(rootNode);
  if (parentNode) {
    if (leftBranch) {
      parentNode->setLeftChild(mapContainerTreeNode);
    } else {
      parentNode->setRightChild(mapContainerTreeNode);
    }
  }

  // Recursively process the remaining nodes
  std::vector<Int>::iterator medianIterator=sortedVector.begin()+medianIndex;
  if (0!=medianIndex) {
    std::vector<Int> t(sortedVector.begin(),medianIterator);
    createSearchTree(mapContainerTreeNode,true,nextDimension,t);
  } else {
    mapContainerTreeNode->setLeftChild(NULL);
  }
  if ((sortedVector.size()-1)!=medianIndex) {
    std::vector<Int> t(medianIterator+1,sortedVector.end());
    createSearchTree(mapContainerTreeNode,false,nextDimension,t);
  } else {
    mapContainerTreeNode->setRightChild(NULL);
  }

  // Return the root node
  return mapContainerTreeNode;
}

// Stores the contents of the search tree in a binary file
void MapSource::storeSearchTree(std::ofstream *ofs, MapContainerTreeNode *node, Int &memorySize) {

  // Calculate the required memory
  memorySize+=sizeof(*node);

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
    storeSearchTree(ofs,node->getLeftChild(),memorySize);
  }

  // Store the right node
  if (node->getRightChild()==NULL) {
    Storage::storeBool(ofs,false);
  } else {
    Storage::storeBool(ofs,true);
    storeSearchTree(ofs,node->getRightChild(),memorySize);
  }

}

// Store the contents of the object in a binary file
void MapSource::store(std::ofstream *ofs, Int &memorySize) {

  Int totalTileCount=0;

  // Calculate the required memory
  memorySize+=sizeof(*this);

  // Write the size of the object for detecting changes later
  Int size=sizeof(*this);
  Storage::storeInt(ofs,size);

  // Store all relevant fields
  Storage::storeString(ofs,folder);
  centerPosition.store(ofs,memorySize);
  currentPosition.store(ofs,memorySize);

  // Store all container objects
  Storage::storeInt(ofs,mapContainers.size());
  for (int i=0;i<mapContainers.size();i++) {
    mapContainers[i]->store(ofs,memorySize);
    totalTileCount+=mapContainers[i]->getTileCount();
  }

  // Store the search trees
  Storage::storeInt(ofs,zoomLevelSearchTrees.size());
  for (int i=0;i<zoomLevelSearchTrees.size();i++) {
    MapContainerTreeNode *startNode=zoomLevelSearchTrees[i];
    if (startNode) {
      Storage::storeBool(ofs,true);
      storeSearchTree(ofs,startNode,memorySize);
    } else {
      Storage::storeBool(ofs,false);
    }
  }

  // Store the size for the progress dialog
  Int progressValueMax=totalTileCount+zoomLevelSearchTrees.size();
  Storage::storeInt(ofs,progressValueMax);
  DEBUG("progressValueMax=%d",progressValueMax);
}

// Reads the contents of the search tree from a binary file
MapContainerTreeNode *MapSource::retrieveSearchTree(MapSource *mapSource, char *&cacheData, Int &cacheSize, char *&objectData, Int &objectSize) {

  // Create a new map container tree node object
  MapContainerTreeNode *mapContainerTreeNode=NULL;
  objectSize-=sizeof(MapContainerTreeNode);
  if (objectSize<0) {
    DEBUG("can not create map container tree node object",NULL);
    return NULL;
  }
  mapContainerTreeNode=new(objectData) MapContainerTreeNode(true);
  objectData+=sizeof(MapContainerTreeNode);

  // Read the current node index and update the contents of the tree node
  Int index;
  Storage::retrieveInt(cacheData,cacheSize,index);
  mapContainerTreeNode->setContents(mapSource->mapContainers[index]);

  // Read the left node
  bool hasLeftNode;
  Storage::retrieveBool(cacheData,cacheSize,hasLeftNode);
  if (hasLeftNode) {
    mapContainerTreeNode->setLeftChild(retrieveSearchTree(mapSource,cacheData,cacheSize,objectData,objectSize));
  }

  // Read the right node
  bool hasRightNode;
  Storage::retrieveBool(cacheData,cacheSize,hasRightNode);
  if (hasRightNode) {
    mapContainerTreeNode->setRightChild(retrieveSearchTree(mapSource,cacheData,cacheSize,objectData,objectSize));
  }

  // Return the node
  return mapContainerTreeNode;
}

// Reads the contents of the object from a binary file
MapSource *MapSource::retrieve(char *&cacheData, Int &cacheSize, char *&objectData, Int &objectSize, std::string folder) {

  PROFILE_START;

  // Check if the class has changed
  Int size=sizeof(MapSource);
#ifdef TARGET_LINUX
  if (size!=528) {
    FATAL("unknown size of object (%d), please adapt class storage",size);
    return NULL;
  }
#endif

  // Read the size of the object and check with current size
  size=0;
  Storage::retrieveInt(cacheData,cacheSize,size);
  if (size!=sizeof(MapSource)) {
    DEBUG("stored size of object does not match implemented object size, aborting retrieve",NULL);
    return NULL;
  }
  PROFILE_ADD("sanity check");

  // Create a new map source object
  MapSource *mapSource=NULL;
  objectSize-=sizeof(MapSource);
  if (objectSize<0) {
    DEBUG("can not create map source object",NULL);
    goto cleanup;
  }
  mapSource=new(objectData) MapSource(true,true);
  mapSource->objectData=objectData;
  objectData+=sizeof(MapSource);
  PROFILE_ADD("object creation");

  // Create a busy dialog
  core->getMapSource()->progressDialogTitle="Reading cache of map " + folder;
  core->getMapSource()->progressValue=0;
  core->getMapSource()->progressIndex=1;
  core->getMapSource()->progressValueMax=*((Int*)&cacheData[cacheSize-sizeof(Int)]);
  core->getMapSource()->progressDialog=core->getDialog()->createProgress(core->getMapSource()->progressDialogTitle,core->getMapSource()->progressValueMax);
  core->getMapSource()->progressUpdateValue=core->getMapSource()->progressIndex*core->getMapSource()->progressValueMax/10;

  // Read the fields
  Storage::retrieveString(cacheData,cacheSize,mapSource->folder);
  MapPosition *p;
  p=MapPosition::retrieve(cacheData,cacheSize,objectData,objectSize);
  if (p==NULL) {
    MapSource::destruct(mapSource);
    mapSource=NULL;
    goto cleanup;
  }
  mapSource->centerPosition=*p;
  MapPosition::destruct(p);
  p=MapPosition::retrieve(cacheData,cacheSize,objectData,objectSize);
  if (p==NULL) {
    MapSource::destruct(mapSource);
    mapSource=NULL;
    goto cleanup;
  }
  mapSource->currentPosition=*p;
  MapPosition::destruct(p);
  PROFILE_ADD("position retrieve");

  // Read the map containers
  Storage::retrieveInt(cacheData,cacheSize,size);
  mapSource->mapContainers.resize(size);
  for (int i=0;i<size;i++) {

    // Retrieve the map container
    MapContainer *c=MapContainer::retrieve(cacheData,cacheSize,objectData,objectSize);
    if (c==NULL) {
      mapSource->mapContainers.resize(i);
      MapSource::destruct(mapSource);
      mapSource=NULL;
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
      MapContainerTreeNode *n=retrieveSearchTree(mapSource,cacheData,cacheSize,objectData,objectSize);
      if (n==NULL) {
        mapSource->zoomLevelSearchTrees.resize(i);
        MapSource::destruct(mapSource);
        mapSource=NULL;
        goto cleanup;
      }
      mapSource->zoomLevelSearchTrees[i]=n;
    }

    // Update the progress
    core->getMapSource()->increaseProgress();

  }
  PROFILE_ADD("search tree retrieve");

  // The progress value was already consumed
  cacheSize-=sizeof(Int);
  cacheData+=sizeof(Int);

  // Object is initialized
  mapSource->setIsInitialized(true);

  // Close the dialog
cleanup:
  core->getDialog()->closeProgress(core->getMapSource()->progressDialog);
  PROFILE_ADD("cleanup");

  PROFILE_END;

  // Return result
  return mapSource;
}

// Destructs the objects correctly (i.e., if memory has not been allocated by new)
void MapSource::destruct(MapSource *object) {
  if (object->doNotDelete) {
    object->~MapSource();
  } else {
    delete object;
  }
}

// Increases the progress by one tep
void MapSource::increaseProgress() {
  progressValue++;
  if (progressValue==progressUpdateValue) {
    core->getDialog()->updateProgress(progressDialog,progressDialogTitle,progressValue);
    progressIndex++;
    progressUpdateValue=progressIndex*progressValueMax/10;
  }
}

}
