//============================================================================
// Name        : MapSource.cpp
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
#include <MapSource.h>
#include <MapPosition.h>
#include <MapSourceEmpty.h>
#include <MapSourceCalibratedPictures.h>
#include <GraphicEngine.h>
#include <MapSourceMercatorTiles.h>
#include <MapArea.h>
#include <MapEngine.h>
#include <Commander.h>
#include <NavigationEngine.h>
#include <Storage.h>

namespace GEODISCOVERER {

// Holds all attributes extracted from the gds file
std::list<ConfigSection*> MapSource::availableGDSInfos;
ConfigSection *MapSource::resolvedGDSInfo = NULL;

// Map source server thread
void *mapSourceRemoteServerThread(void *args) {
  MapSource *mapSource = (MapSource*)args;
  mapSource->remoteServer();
  return NULL;
}

MapSource::MapSource() {
  folder=core->getConfigStore()->getStringValue("Map","folder", __FILE__, __LINE__);
  neighborPixelTolerance=core->getConfigStore()->getDoubleValue("Map","neighborPixelTolerance", __FILE__, __LINE__);
  mapTileLength=core->getConfigStore()->getIntValue("Map","tileLength", __FILE__, __LINE__);
  statusMutex=core->getThread()->createMutex("map source status mutex");
  isInitialized=false;
  contentsChanged=false;
  centerPosition=NULL;
  mapArchivesMutex=core->getThread()->createMutex("map archives mutex");
  mapDownloader=NULL;
  minZoomLevel=-1;
  maxZoomLevel=-1;
  std::string defaultLegendPath=core->getHomePath() + "/Map/" + folder + "/legend.png";
  if (access(defaultLegendPath.c_str(),F_OK)==0)
    legendPaths[getFolder()]=defaultLegendPath;
  quitRemoteServerThread=false;
  remoteServerStartSignal=NULL;
  remoteServerThreadInfo=NULL;
  resetRemoteServerThread=false;
}

MapSource::~MapSource() {
  deinit();
  quitRemoteServerThread=true;
  if (remoteServerThreadInfo) {
    core->getThread()->issueSignal(remoteServerStartSignal);
    core->getThread()->waitForThread(remoteServerThreadInfo);
    core->getThread()->destroyThread(remoteServerThreadInfo);
    core->getThread()->destroySignal(remoteServerStartSignal);
  }
  core->getThread()->destroyMutex(statusMutex);
  core->getThread()->destroyMutex(mapArchivesMutex);
}

// Clear the source
void MapSource::deinit()
{
  for (std::vector<MapContainer*>::const_iterator i=mapContainers.begin();i!=mapContainers.end();i++) {
    MapContainer::destruct(*i);
  }
  mapContainers.clear();
  if (centerPosition) {
    MapPosition::destruct(centerPosition);
    centerPosition=NULL;
  }
  for(std::vector<MapContainerTreeNode*>::iterator i=zoomLevelSearchTrees.begin();i!=zoomLevelSearchTrees.end();i++) {
    MapContainerTreeNode *t=*i;
    if (t)
      delete t;
  }
  zoomLevelSearchTrees.clear();
  for(std::list<ZipArchive*>::iterator i=mapArchives.begin();i!=mapArchives.end();i++) {
    delete *i;
  }
  mapArchives.clear();
  GraphicObject *pathAnimators=core->getDefaultGraphicEngine()->lockPathAnimators(__FILE__, __LINE__);
  for (std::list<GraphicPrimitiveKey>::iterator i=retrievedPathAnimators.begin();i!=retrievedPathAnimators.end();i++) {
    pathAnimators->removePrimitive(*i,true);
  }
  core->getDefaultGraphicEngine()->unlockPathAnimators();
  retrievedPathAnimators.clear();
  isInitialized=false;
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

// Inserts a new map container in the search tree
void MapSource::insertNodeIntoSearchTree(MapContainer *newMapContainer, Int zoomLevel, MapContainerTreeNode* prevMapContainerTreeNode, bool useRightChild, GeographicBorder currentDimension) {

  GeographicBorder nextDimension;

  // Get the current map container
  MapContainerTreeNode *currentMapContainerTreeNode;
  if (prevMapContainerTreeNode) {
    if (useRightChild)
      currentMapContainerTreeNode=prevMapContainerTreeNode->getRightChild();
    else
      currentMapContainerTreeNode=prevMapContainerTreeNode->getLeftChild();
  } else {
    currentMapContainerTreeNode=zoomLevelSearchTrees[zoomLevel];
  }

  // Insert node if no new node could be found
  if (!currentMapContainerTreeNode) {
    MapContainerTreeNode *newMapContainerTreeNode=new MapContainerTreeNode();
    if (!newMapContainerTreeNode) {
      FATAL("can not create map container tree node",NULL);
      return;
    }
    newMapContainerTreeNode->setContents(newMapContainer);
    if (prevMapContainerTreeNode==NULL) {
      zoomLevelSearchTrees[zoomLevel]=newMapContainerTreeNode;
    } else {
      if (useRightChild)
        prevMapContainerTreeNode->setRightChild(newMapContainerTreeNode);
      else
        prevMapContainerTreeNode->setLeftChild(newMapContainerTreeNode);
    }
    return;
  }

  // Decide with which branches of the tree to continue
  MapContainer *currentMapContainer;
  currentMapContainer=currentMapContainerTreeNode->getContents();
  switch(currentDimension) {
    case GeographicBorderLatNorth:
      nextDimension=GeographicBorderLatSouth;
      if (newMapContainer->getLatNorth()>currentMapContainer->getLatNorth()) {
        useRightChild=true;
      } else {
        useRightChild=false;
      }
      break;
    case GeographicBorderLatSouth:
      nextDimension=GeographicBorderLngEast;
      if (newMapContainer->getLatSouth()<currentMapContainer->getLatSouth()) {
        useRightChild=false;
      } else {
        useRightChild=true;
      }
      break;
    case GeographicBorderLngEast:
      nextDimension=GeographicBorderLngWest;
      if (newMapContainer->getLngEast()>currentMapContainer->getLngEast()) {
        useRightChild=true;
      } else {
        useRightChild=false;
      }
      break;
    case GeographicBorderLngWest:
      nextDimension=GeographicBorderLatNorth;
      if (newMapContainer->getLngWest()<currentMapContainer->getLngWest()) {
        useRightChild=false;
      } else {
        useRightChild=true;
      }
      break;
  }

  // Get the best tiles from the left and the right branch
  insertNodeIntoSearchTree(newMapContainer,zoomLevel,currentMapContainerTreeNode,useRightChild,nextDimension);
}

// Initializes the progress bar
void MapSource::openProgress(std::string title, Int valueMax) {
  core->getMapSource()->progressDialogTitle="Reading cache of map " + folder;
  core->getMapSource()->progressValue=0;
  core->getMapSource()->progressIndex=1;
  core->getMapSource()->progressValueMax=valueMax;
  core->getMapSource()->progressDialog=core->getDialog()->createProgress(core->getMapSource()->progressDialogTitle,core->getMapSource()->progressValueMax);
  core->getMapSource()->progressUpdateValue=core->getMapSource()->progressIndex*core->getMapSource()->progressValueMax/10;
}

// Increases the progress by one tep
bool MapSource::increaseProgress() {

  // Update the progress
  progressValue++;
  if (progressValue==progressUpdateValue) {
    core->getDialog()->updateProgress(progressDialog,progressDialogTitle,progressValue);
    progressIndex++;
    progressUpdateValue=progressIndex*progressValueMax/10;
  }

  // Shall we stop?
  if (core->getQuitCore()) {
    return false;
  } else {
    return true;
  }

}

// Closes the progress bar
void MapSource::closeProgress() {
  core->getDialog()->closeProgress(progressDialog);
}

// Renames the layers with the infos in the gds file
void MapSource::renameLayers() {

  // Rename the map layers
  std::string infoFilePath = getFolderPath() + "/info.gds";
  std::list<XMLNode> layerNames = resolvedGDSInfo->findConfigNodes("/GDS/LayerName");
  //DEBUG("size=%d",layerNames.size());
  for(std::list<XMLNode>::iterator i=layerNames.begin();i!=layerNames.end();i++) {
    bool zoomLevelFound;
    Int zoomLevel;
    zoomLevelFound=resolvedGDSInfo->getNodeText(*i,"zoomLevel",zoomLevel);
    bool nameFound;
    std::string name;
    nameFound=resolvedGDSInfo->getNodeText(*i,"name",name);
    if (!zoomLevelFound) {
      ERROR("one LayerName element has no zoomLevel element in <%s>",infoFilePath.c_str());
      break;
    }
    if (!nameFound) {
      ERROR("one LayerName element has no name element in <%s>",infoFilePath.c_str());
      break;
    }
    //DEBUG("name=%s zoomLevel=%d",name.c_str(),zoomLevel);
    bool found=false;
    for (MapLayerNameMap::iterator i=mapLayerNameMap.begin();i!=mapLayerNameMap.end();i++) {
      if (i->second==zoomLevel-minZoomLevel+1) {
        mapLayerNameMap.erase(i);
        mapLayerNameMap[name]=zoomLevel-minZoomLevel+1;
        found=true;
        break;
      }
    }
    if (!found) {
      ERROR("zoom level %d of map layer with name <%s> does not exist",zoomLevel,name.c_str());
    }
  }
}

// Creates a new map source object of the correct type
MapSource *MapSource::newMapSource() {

  std::string folder = core->getConfigStore()->getStringValue("Map","folder", __FILE__, __LINE__);
  std::string folderPath = core->getHomePath() + "/Map/" + folder;
  std::string infoPath = folderPath + "/info.gds";
  TimestampInSeconds lastGDSModification=0;

  // Check if the folder exists
  struct stat s;
  int err = core->statFile(folderPath, &s);
  if ((err!=0)||(!S_ISDIR(s.st_mode))) {
    ERROR("map folder <%s> does not exist",folder.c_str());
    return new MapSourceEmpty();
  }

  // If the info.gds file does not exist, it is an offline source
  std::string type = "tileServer";
  std::list<std::string> mapArchivePaths;
  bool mapArchivePathFound = false;
  if (access(infoPath.c_str(),F_OK)) {

    type="internalMapArchive";

  } else {

    // Read in info.gds to find out type of source
    readAvailableGDSInfos();
    if (resolvedGDSInfo) {
      delete resolvedGDSInfo;
      resolvedGDSInfo=NULL;
    }
    resolveGDSInfo(infoPath,&lastGDSModification);

    // Find out the kind of source
    resolvedGDSInfo->getNodeText("/GDS/type",type);

    // Add all map archives
    std::list<XMLNode> mapArchivePathNodes=resolvedGDSInfo->findConfigNodes("/GDS/mapArchivePath");
    for(std::list<XMLNode>::iterator i=mapArchivePathNodes.begin();i!=mapArchivePathNodes.end();i++) {
      std::string t;
      if (resolvedGDSInfo->getNodeText(*i,t)) {
        mapArchivePaths.push_back(t);
        mapArchivePathFound = true;
      }
    }
  }

  // Internal map archive?
  MapSource *result = NULL;
  if (type=="internalMapArchive") {

    // Get all the tiles of the map
    std::list<std::string> mapArchivePaths;
    struct dirent *dp;
    DIR *dfd;
    dfd=core->openDir(folderPath);
    if (dfd==NULL) {
      FATAL("can not read directory <%s>",folderPath.c_str());
      return new MapSourceEmpty();
    }
    while ((dp = readdir(dfd)) != NULL)
    {
      Int nr;

      // Add this archive if it is valid
      if ((sscanf(dp->d_name,"tiles%d.gda",&nr)==1)||(strcmp(dp->d_name,"tiles.gda")==0)) {
        mapArchivePaths.push_back(folderPath + "/" + dp->d_name);
      }
    }
    closedir(dfd);

    // Check if tiles archive exists
    if (mapArchivePaths.size()==0) {
      ERROR("map folder <%s> does not contain a tiles.gda file",NULL);
      return new MapSourceEmpty();
    } else {
      result = new MapSourceCalibratedPictures(mapArchivePaths);
    }

  }

  // Online source?
  if (type=="tileServer") {
    result = new MapSourceMercatorTiles(lastGDSModification);
  }

  // External map archive?
  if (type=="externalMapArchive") {
    if (!mapArchivePathFound) {
      ERROR("map source file <%s> does not contain a mapArchivePath element",infoPath.c_str());
      return new MapSourceEmpty();
    }
    for (std::list<std::string>::iterator i=mapArchivePaths.begin();i!=mapArchivePaths.end();i++) {
      if (access((*i).c_str(),F_OK)) {
        ERROR("external map archive <%s> referenced in <%s> does not exist",(*i).c_str(),infoPath.c_str());
        return new MapSourceEmpty();
      }
    }
    result = new MapSourceCalibratedPictures(mapArchivePaths);
  }

  // Was a map source created?
  if (result==NULL) {
    ERROR("map source type <%s> is not supported",type.c_str());
    return new MapSourceEmpty();
  }

  // Return the result
  return result;
}

// Returns the map tile in which the position lies
MapTile *MapSource::findMapTileByGeographicCoordinate(MapPosition pos, Int zoomLevel, bool lockZoomLevel, MapContainer *preferredMapContainer) {

  bool betterMapContainerFound=false;
  MapPosition bestPos;
  double distToNearestLngScale=-1, distToNearestLatScale=-1;

  // Abort if no search tree is available
  if (zoomLevelSearchTrees.size()==0)
    return NULL;

  // Reset the zoom level if it is out of range
  if (zoomLevel>zoomLevelSearchTrees.size()-1)
    zoomLevel=0;

  // Get the search tree to use
  MapContainerTreeNode *startNode=zoomLevelSearchTrees[zoomLevel];

  // First find the map container matching the given position
  MapContainer *foundMapContainer=findMapContainerByGeographicCoordinate(pos,preferredMapContainer,startNode,GeographicBorderLatNorth,bestPos,distToNearestLngScale,distToNearestLatScale,betterMapContainerFound);
  //DEBUG("found map container = %08x",foundMapContainer);

  // If no container can be found, try to fall back to zoom level 0 (all tiles)
  if ((!foundMapContainer)&&(zoomLevel!=0)&&(!lockZoomLevel)) {
    startNode=zoomLevelSearchTrees[0];
    foundMapContainer=findMapContainerByGeographicCoordinate(pos,preferredMapContainer,startNode,GeographicBorderLatNorth,bestPos,distToNearestLngScale,distToNearestLatScale,betterMapContainerFound);
  }

  // Now search the closest tile in the map
  if (foundMapContainer) {
    return foundMapContainer->findMapTileByPictureCoordinate(bestPos);
  } else {
    return NULL;
  }
}

// Returns the map tile in which the position lies
MapContainer *MapSource::findMapContainerByGeographicCoordinate(MapPosition pos, MapContainer *preferredMapContainer, MapContainerTreeNode* currentMapContainerTreeNode, GeographicBorder currentDimension, MapPosition &bestPos, double &distToNearestLngScale, double &distToNearestLatScale, bool &betterMapContainerFound, std::list<MapContainer*> *foundMapContainers) {

  MapContainer *bestMapContainer=NULL,*bestMapContainerLeft=NULL,*bestMapContainerRight=NULL;
  bool betterMapContainerFoundRight=false,betterMapContainerFoundLeft=false;
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
        double distToLngScale=fabs(currentMapContainer->getLngScale()-(pos.getLngScale()));
        double distToLatScale=fabs(currentMapContainer->getLatScale()-(pos.getLatScale()));
        //DEBUG("map=%s map.lngSccale=%f pos.lngScale=%f",currentMapContainer->getImageFileName().c_str(),currentMapContainer->getLngScale(),pos.getLngScale());

        // Shall we return all matching containers?
        if (foundMapContainers) {
          foundMapContainers->push_back(currentMapContainer);
        }

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
          //DEBUG("new candidate found: zoomLevel=%d distToNearestLngScale=%f distToNearestLatScale=%f",currentMapContainer->getZoomLevel(),distToNearestLngScale,distToNearestLatScale);

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
    bestMapContainerRight=findMapContainerByGeographicCoordinate(pos,preferredMapContainer,currentMapContainerTreeNode->getRightChild(),nextDimension,bestPosRight,distToNearestLngScaleRight,distToNearestLatScaleRight,betterMapContainerFoundRight,foundMapContainers);
  }
  distToNearestLatScaleLeft=distToNearestLatScaleRight;
  distToNearestLngScaleLeft=distToNearestLngScaleRight;
  bestPosLeft=bestPosRight;
  if ((useLeftChild)&&(!preferredMapContainer||bestMapContainerRight!=preferredMapContainer)) {
    bestMapContainerLeft=findMapContainerByGeographicCoordinate(pos,preferredMapContainer,currentMapContainerTreeNode->getLeftChild(),nextDimension,bestPosLeft,distToNearestLngScaleLeft,distToNearestLatScaleLeft,betterMapContainerFoundLeft,foundMapContainers);
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

  // Abort if no search tree is available
  if (zoomLevelSearchTrees.size()==0)
    return NULL;

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
    if (area.getZoomLevel()==currentMapContainer->getZoomLevelMap()) {
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

  // Abort if no search tree is available
  if (zoomLevelSearchTrees.size()==0)
    return result;

  // Get the search tree to use
  MapContainerTreeNode *startNode=zoomLevelSearchTrees[area.getZoomLevel()];

  // Find all map containers matching the given area
  findMapContainerByGeographicArea(area,NULL,startNode,GeographicBorderLatNorth,bestDistance,bestTranslatedArea,betterMapContainerFound,&result);
  return result;
}

// Returns a list of map containers in which the given position lies
std::list<MapContainer*> MapSource::findMapContainersByGeographicCoordinate(MapPosition pos, Int zoomLevel) {

  bool betterMapContainerFound=false;
  MapPosition bestPos;
  double distToNearestLngScale=-1, distToNearestLatScale=-1;
  std::list<MapContainer*> result;

  // Do not work if the source has no search trees
  if (zoomLevelSearchTrees.size()==0)
    return result;

  // Get the search tree to use
  MapContainerTreeNode *startNode=zoomLevelSearchTrees[zoomLevel];

  // Find all map containers matching the given area
  findMapContainerByGeographicCoordinate(pos,NULL,startNode,GeographicBorderLatNorth,bestPos,distToNearestLngScale,distToNearestLatScale,betterMapContainerFound,&result);
  return result;
}

// Performs maintenance tasks
void MapSource::maintenance() {
}

// Recreates the search data structures
void MapSource::createSearchDataStructures(bool showProgressDialog) {

  MapContainer *mapContainer;
  std::string title;
  DialogKey dialog;
  Int progress;

  // Find out the max zoom level
  Int maxZoomLevel = zoomLevelSearchTrees.size() - 1;

  // Open dialog if requested
  if (showProgressDialog) {
    title="Creating search tree for map " + folder;
    progress=1;
    dialog=core->getDialog()->createProgress(title,maxZoomLevel+1);
  }

  // Create the sorted vectors for search tree creation
  mapsIndexByLatNorth.clear();
  mapsIndexByLatSouth.clear();
  mapsIndexByLngEast.clear();
  mapsIndexByLngWest.clear();
  for (Int i=0;i<mapContainers.size();i++) {
    mapContainer=mapContainers[i];
    insertMapContainerToSortedList(&mapsIndexByLatNorth,mapContainer,i,GeographicBorderLatNorth);
    insertMapContainerToSortedList(&mapsIndexByLatSouth,mapContainer,i,GeographicBorderLatSouth);
    insertMapContainerToSortedList(&mapsIndexByLngWest,mapContainer,i,GeographicBorderLngWest);
    insertMapContainerToSortedList(&mapsIndexByLngEast,mapContainer,i,GeographicBorderLngEast);
  }

  // Prepare the masking vector for each zoom level
  std::vector< std::vector<Int> > mapsIndexByZoomLevel;
  for (int i=0;i<=maxZoomLevel;i++) {
    mapsIndexByZoomLevel.push_back(std::vector<int>());
  }
  for(std::vector<Int>::iterator i=mapsIndexByLatNorth.begin();i!=mapsIndexByLatNorth.end();i++) {
    Int index=*i;
    mapContainer=mapContainers[index];
    Int zoomLevel=mapContainer->getZoomLevelMap();
    mapsIndexByZoomLevel[zoomLevel].push_back(index);
    mapsIndexByZoomLevel[0].push_back(index);  // zoom level 0 contains all containers and tiles
  }

  // Clear the current search trees
  for(std::vector<MapContainerTreeNode*>::iterator i=zoomLevelSearchTrees.begin();i!=zoomLevelSearchTrees.end();i++) {
    MapContainerTreeNode *t=*i;
    if (t)
      delete t;
    *i=NULL;
  }

  // Create the search trees for each zoom level
  for(Int i=0;i<=maxZoomLevel;i++) {
    if (showProgressDialog)
      core->getDialog()->updateProgress(dialog,title,progress);
    std::vector<Int> zoomLevelMapsIndex=mapsIndexByZoomLevel[i];
    if (zoomLevelMapsIndex.size()>0)
      zoomLevelSearchTrees[i]=createSearchTree(NULL,false,GeographicBorderLatNorth,zoomLevelMapsIndex);
    progress++;
  }
  if (showProgressDialog)
    core->getDialog()->closeProgress(dialog);

}

// Marks a map container as obsolete
// Please note that other objects might still use this map container
// Call unlinkMapContainer to solve this afterwards
void MapSource::markMapContainerObsolete(MapContainer *c) {
}

// Removes all obsolete map containers
void MapSource::removeObsoleteMapContainers(MapArea *displayArea, bool allZoomLevels) {
}

// Returns the names of each map layer
std::list<std::string> MapSource::getMapLayerNames() {
  std::list<std::string> result;
  for (Int z=zoomLevelSearchTrees.size()-1;z>0;z--) {
    for (MapLayerNameMap::iterator i=mapLayerNameMap.begin();i!=mapLayerNameMap.end();i++) {
      if (i->second==z) result.push_back(i->first);
    }
  }
  return result;
}

// Returns the currently selected map layer
std::string MapSource::getSelectedMapLayer() {
  MapLayerNameMap::iterator i;
  Int selectedZoomLevel = core->getMapEngine()->getZoomLevel();
  for (i=mapLayerNameMap.begin();i!=mapLayerNameMap.end();i++) {
    if (i->second==selectedZoomLevel)
      return i->first;
  }
  return "";
}

// Selects the given map layer
void MapSource::selectMapLayer(std::string name) {
  MapLayerNameMap::iterator i=mapLayerNameMap.find(name);
  if (i!=mapLayerNameMap.end()) {
    MapPosition pos=*(core->getMapEngine()->lockMapPos(__FILE__,__LINE__));
    core->getMapEngine()->unlockMapPos();
    core->getMapSource()->lockAccess(__FILE__,__LINE__);
    bool found=core->getMapSource()->findMapTileByGeographicCoordinate(pos,i->second,true);
    core->getMapSource()->unlockAccess();
    if (found) {
      core->getMapEngine()->setZoomLevel(i->second);
    } else {
      WARNING("map layer <%s> is not selected because it has no tile for current position",name.c_str());
    }
  }
}

// Returns the name of the given zoom level
std::string MapSource::getMapLayerName(int zoomLevel) {
  for (MapLayerNameMap::iterator i=mapLayerNameMap.begin();i!=mapLayerNameMap.end();i++) {
    if (i->second==zoomLevel) return i->first;
  }
  return "";
}

// Adds a download job from the current visible map
void MapSource::addDownloadJob(bool estimateOnly, std::string routeName, std::string zoomLevels) {
}

// Indicates if the map source has download jobs
bool MapSource::hasDownloadJobs() {
  return false;
}

// Processes all pending download jobs
void MapSource::processDownloadJobs() {
}

// Returns the number of unqueued but not downloaded tiles
Int MapSource::countUnqueuedDownloadTiles(bool peek) {
  return 0;
}

// Triggers the download job processing
void MapSource::triggerDownloadJobProcessing() {
}

// Removes all download jobs
void MapSource::clearDownloadJobs() {
}

// Ensures that all threads that download tiles are stopped
void MapSource::stopDownloadThreads() {
}

// Returns a list of names of the maps that have a legend
std::list<std::string> MapSource::getLegendNames() {
  std::list<std::string> l;
  for (StringMap::iterator i=legendPaths.begin();i!=legendPaths.end();i++)
    l.push_back(i->first);
  return l;
}

// Returns the file path to the legend with the given name
std::string MapSource::getLegendPath(std::string name) {
  return legendPaths[name];
}

// Finds the calibrator for the given position
MapCalibrator *MapSource::findMapCalibrator(Int zoomLevel, MapPosition pos, bool &deleteCalibrator) {
  deleteCalibrator=false;
  MapTile *tile=findMapTileByGeographicCoordinate(pos,zoomLevel,true,NULL);
  if (tile)
    return tile->getParentMapContainer()->getMapCalibrator();
  else
    return NULL;
}

// Returns the scale values for the given zoom level
void MapSource::getScales(Int zoomLevel, double &latScale, double &lngScale) {
  if (zoomLevelSearchTrees[zoomLevel]!=NULL) {
    MapContainer *c=zoomLevelSearchTrees[zoomLevel]->getContents();
    lngScale=c->getLngScale();
    latScale=c->getLatScale();
  } else {
    lngScale=1;
    latScale=1;
  }
}

// Update any wear device about the new map
void MapSource::remoteMapInit() {

  // Start the server (if not done already)
  resetRemoteServerThread=true;
  if ((remoteServerThreadInfo==NULL)&&((type==MapSourceTypeCalibratedPictures)||(type==MapSourceTypeMercatorTiles))) {
    DEBUG("starting remote server thread",NULL);
    remoteServerStartSignal=core->getThread()->createSignal();
    remoteServerThreadInfo=core->getThread()->createThread("map source remote server thread",mapSourceRemoteServerThread,(void*)this);
  }

  // Inform the remote device
  std::stringstream cmd;
  MapPosition pos=*(core->getMapEngine()->lockMapPos(__FILE__,__LINE__));
  core->getMapEngine()->unlockMapPos();
  cmd << "setNewRemoteMap"
      << "(Map/Remote,folder," << folder << ",1)"
      << "(Map/Remote,minZoomLevel," << minZoomLevel << ",1)"
      << "(Map/Remote,maxZoomLevel," << maxZoomLevel << ",1)"
      << "(Map/Remote,centerLng," << pos.getLng() << ",0)"
      << "(Map/Remote,centerLat," << pos.getLat() << ",0)"
      << "(Map/Remote,centerLngScale," << pos.getLngScale() << ",0)"
      << "(Map/Remote,centerLatScale," << pos.getLatScale() << ",0)";
  core->getCommander()->dispatch(cmd.str());
  DEBUG(cmd.str().c_str(),NULL);
}

// Adds a new command for the remote server
void MapSource::queueRemoteServerCommand(std::string cmd) {
  if (!remoteServerStartSignal)
    return;
  lockAccess(__FILE__,__LINE__);
  remoteServerCommandQueue.push_back(cmd);
  unlockAccess();
  core->getThread()->issueSignal(remoteServerStartSignal);
}

// Adds the given map container to the queue for sending to the remote server
void MapSource::queueRemoteMapContainer(MapContainer* c, std::vector<std::string> *alreadyKnownMapContainers, Int startIndex, std::list<std::vector<std::string> > *mapImagesToServe) {

  std::string workPath = core->getHomePath() + "/Map";

  // Does the remote side already know this one?
  bool alreadyKnown = false;
  if (alreadyKnownMapContainers) {
    for (int i = startIndex; i < alreadyKnownMapContainers->size(); i+=2) {
      //DEBUG("mapContainer=%s remoteHash=%s",(*alreadyKnownMapContainers)[i].c_str(),(*alreadyKnownMapContainers)[i+1].c_str());
      if ((*alreadyKnownMapContainers)[i] == c->getCalibrationFilePath()) {
        //DEBUG("remote side already has <%s>, skipping it",c->getCalibrationFilePath());
        alreadyKnown = true;

        // If the overlay hash is not yet computed, create it
        if (c->getOverlayGraphicHash()=="") {
          c->storeOverlayGraphics(workPath,"remoteOverlay.gdo");
          remove((workPath + "/remoteOverlay.gda").c_str());
        }

        // Did the overlay change?
        //DEBUG("remoteHash=%s localHash=%s",(*alreadyKnownMapContainers)[i+1].c_str(),c->getOverlayGraphicHash().c_str());
        if ((*alreadyKnownMapContainers)[i+1]!=c->getOverlayGraphicHash()) {
          DEBUG("overlay for <%s> outdated at remote side, adding it to queue", c->getCalibrationFileName());
          queueRemoteServerCommand("serveRemoteMapOverlay(" + std::string(c->getCalibrationFilePath()) + ")");
        }
      }
    }
  }

  // If not, serve it add it to the work queue
  if (!alreadyKnown) {

    // If the container is not yet downloaded, indicate that this must be delivered to remote side
    if (!c->getDownloadComplete()) {
      c->setServeToRemoteMap(true);
      //DEBUG("map container %s not yet downloaded, requesting delivery after download", c->getCalibrationFileName());
    } else {
      std::vector<std::string> mapImage;
      mapImage.push_back(c->getImageFilePath());
      mapImage.push_back(c->getCalibrationFilePath());
      mapImage.push_back(c->getArchiveFileFolder());
      mapImage.push_back(c->getArchiveFileName());
      mapImagesToServe->push_back(mapImage);
      queueRemoteServerCommand("serveRemoteMapOverlay(" + std::string(c->getCalibrationFilePath()) + ")");
      //DEBUG("map container %s added to serve queue", c->getCalibrationFileName());
    }
  }
}

// Updates the navigation engine overlay archive if necessary
void MapSource::queueRemoteNavigationEngineOverlayArchive(std::string remoteHash) {

  std::string workPath = core->getHomePath() + "/Map";

  // If the overlay hash is not yet computed, create it
  if (core->getNavigationEngine()->getOverlayGraphicHash()=="") {
    core->getNavigationEngine()->storeOverlayGraphics(workPath,"navigationEngine.gdo");
  }

  // Did the overlay change?
  //DEBUG("remoteHash=%s localHash=%s",(*alreadyKnownMapContainers)[i+1].c_str(),c->getOverlayGraphicHash().c_str());
  if (remoteHash!=core->getNavigationEngine()->getOverlayGraphicHash()) {
    DEBUG("navigation engine overlay out dated at remote side, adding it to queue", NULL);
    queueRemoteServerCommand("serveRemoteNavigationEngineOverlay(" + workPath + "/navigationEngine.gdo)");
  }

}

// Handles request from remote devices for tiles
void MapSource::remoteServer() {

  std::string workPath = core->getHomePath() + "/Map";
  std::list<std::string> servedMapContainers;
  std::list<std::string> servedOverlays;

  // Set the priority
  core->getThread()->setThreadPriority(threadPriorityBackgroundLow);

  // Delete any left over remote tiles
  Int tileNr=0;
  struct dirent *dp;
  DIR *dfd;
  dfd=core->openDir(workPath);
  if (dfd==NULL) {
    FATAL("can not read directory <%s>",workPath.c_str());
    return;
  }
  std::list<std::string> leftoverRemoteTiles;
  while ((dp = readdir(dfd)) != NULL)
  {
    if (sscanf(dp->d_name,"remoteTile%d.gda",&tileNr)==1) {
      leftoverRemoteTiles.push_back(dp->d_name);
    }
  }
  closedir(dfd);
  tileNr=0;
  for (std::list<std::string>::iterator i=leftoverRemoteTiles.begin();i!=leftoverRemoteTiles.end();i++) {
    std::string path = workPath + "/" + *i;
    remove(path.c_str());
  }

  // Delete any left over remote overlays
  Int overlayNr=0;
  Int x,y;
  dfd=core->openDir(workPath);
  if (dfd==NULL) {
    FATAL("can not read directory <%s>",workPath.c_str());
    return;
  }
  std::list<std::string> leftoverRemoteOverlays;
  while ((dp = readdir(dfd)) != NULL)
  {
    if (sscanf(dp->d_name,"remoteOverlay%d.gdo",&overlayNr)==1) {
      leftoverRemoteOverlays.push_back(dp->d_name);
    }
  }
  closedir(dfd);
  overlayNr=0;
  for (std::list<std::string>::iterator i=leftoverRemoteOverlays.begin();i!=leftoverRemoteOverlays.end();i++) {
    std::string path = workPath + "/" + *i;
    remove(path.c_str());
  }

  // Delete any left over navigation engine overlay
  remove((workPath + "/navigationEngine.gdo").c_str());

  // Do an endless loop
  while (1) {

    // Wait for trigger
    core->getThread()->waitForSignal(remoteServerStartSignal);

    // Shall we quit?
    if (quitRemoteServerThread) {
      core->getThread()->exitThread();
    }

    // Repeat until queue is empty
    while(!remoteServerCommandQueue.empty()) {

      // Get command from queue
      lockAccess(__FILE__,__LINE__);
      std::string cmd=remoteServerCommandQueue.front();
      remoteServerCommandQueue.pop_front();
      unlockAccess();

      // Split the command
      DEBUG("new cmd: %s",cmd.c_str());
      std::string cmdName;
      std::vector<std::string> args;
      if (!core->getCommander()->splitCommand(cmd,cmdName,args)) {
        FATAL("command %s can not be splitted",cmd.c_str());
      }
      bool commandProcessed=false;

      // Reset requested?
      if (resetRemoteServerThread) {
        servedMapContainers.clear();
        servedOverlays.clear();
        resetRemoteServerThread=false;
      }

      // Create the list of map images to serve
      std::list<std::vector<std::string> > mapImagesToServe;

      // Was a remote map served?
      if (cmdName=="remoteMapArchiveServed") {
        servedMapContainers.remove(args[0]);
        DEBUG("servedMapContainers.size()=%d",servedMapContainers.size());
        commandProcessed=true;
      }

      // Was a remote overlay served?
      if (cmdName=="remoteOverlayArchiveServed") {
        servedOverlays.remove(args[0]);
        DEBUG("servedOverlays.size()=%d",servedOverlays.size());
        commandProcessed=true;
      }

      // Shall we serve a navigation engine overlay?
      if (cmdName=="serveRemoteNavigationEngineOverlay") {

        // Check if the overlay is already beeing served
        std::string hash=core->getNavigationEngine()->getOverlayGraphicHash();
        bool alreadyServed=false;
        for (std::list<std::string>::iterator i=servedOverlays.begin();i!=servedOverlays.end();i++) {
          if (*i==hash) {
            DEBUG("requested navigation overlay already being served, skipping it", NULL);
            alreadyServed=true;
          }
        }
        if (alreadyServed)
          continue;

        // Create the overlay and queue it for sending
        core->getCommander()->dispatch("serveRemoteOverlayArchive(" + args[0] + "," + hash + "," + hash + ")");
        servedOverlays.push_back(hash);
        commandProcessed=true;
      }

      // Shall we serve a map containr that was not yet downloaded?
      if (cmdName=="serveRemoteMapOverlay") {

        // Find the map container
        MapContainer *c=NULL;
        lockAccess(__FILE__,__LINE__);
        for (std::vector<MapContainer*>::iterator i=mapContainers.begin();i!=mapContainers.end();i++) {
          if ((*i)->getCalibrationFilePath()==args[0]) {
            c=*i;
            break;
          }
        }
        unlockAccess();
        if (!c) {
          FATAL("map container %s could not be found",args[0].c_str());
          continue;
        }

        // Check if the overlay is already beeing served
        bool alreadyServed=false;
        for (std::list<std::string>::iterator i=servedOverlays.begin();i!=servedOverlays.end();i++) {
          if (*i==c->getOverlayGraphicHash()) {
            DEBUG("requested overlay <%s> already being served, skipping it", c->getCalibrationFilePath());
            alreadyServed=true;
          }
        }
        if (alreadyServed)
          continue;

        // Create the overlay and queue it for sending
        std::stringstream remoteOverlayFilename;
        remoteOverlayFilename << "remoteOverlay" << overlayNr << ".gdo";
        overlayNr++;
        c->storeOverlayGraphics(workPath,remoteOverlayFilename.str());
        std::string hash = c->getOverlayGraphicHash();
        core->getCommander()->dispatch("serveRemoteOverlayArchive(" + workPath + "/" + remoteOverlayFilename.str() + "," + hash + "," + hash + ")");
        servedOverlays.push_back(hash);
        commandProcessed=true;
      }

      // Shall we serve a map containr that was not yet downloaded?
      if (cmdName=="serveRemoteMapContainer") {

        // Find the map container
        MapContainer *c=NULL;
        lockAccess(__FILE__,__LINE__);
        for (std::vector<MapContainer*>::iterator i=mapContainers.begin();i!=mapContainers.end();i++) {
          if ((*i)->getCalibrationFilePath()==args[0]) {
            c=*i;
            break;
          }
        }
        if (!c) {
          FATAL("map container %s could not be found",args[0].c_str());
        } else {
          queueRemoteMapContainer(c,NULL,0,&mapImagesToServe);
        }
        unlockAccess();
        commandProcessed=true;
      }

      // Shall we provide a map tile from a geo area?
      if (cmdName=="fillGeographicAreaWithRemoteTiles") {

        // Ensure that the map engine works and updates overlays (if screen is off)
        if ((core->getNavigationEngine()->mapGraphicUpdateIsRequired(__FILE__,__LINE__))) {
          core->getNavigationEngine()->updateMapGraphic();
        }

        // Convert the arguments
        double lng=atof(args[0].c_str());
        double lat=atof(args[1].c_str());
        double lngScale=atof(args[2].c_str());
        double latScale=atof(args[3].c_str());
        Int zoomLevel=atoi(args[4].c_str());
        Int refX=atoi(args[5].c_str());
        Int refY=atoi(args[6].c_str());
        Int yNorth=atoi(args[7].c_str());
        Int ySouth=atoi(args[8].c_str());
        Int xEast=atoi(args[9].c_str());
        Int xWest=atoi(args[10].c_str());
        double latNorth=atof(args[11].c_str());
        double latSouth=atof(args[12].c_str());
        double lngEast=atof(args[13].c_str());
        double lngWest=atof(args[14].c_str());
        Int maxTiles=atoi(args[15].c_str());
        std::string preferredNeighborCalibrationPath=args[16];
        Int mapX=atoi(args[17].c_str());
        Int mapY=atoi(args[18].c_str());
        std::string navigationEngineOverlayHash=args[19];
        MapPosition refPos;
        refPos.setLng(lng);
        refPos.setLat(lat);
        refPos.setLngScale(lngScale);
        refPos.setLatScale(latScale);
        refPos.setX(refX);
        refPos.setY(refY);
        //DEBUG("x=%d y=%d lngScale=%f latScale=%f",refPos.getX(),refPos.getY(),refPos.getLngScale(),refPos.getLatScale());
        MapArea area;
        area.setRefPos(refPos);
        area.setZoomLevel(zoomLevel);
        area.setYNorth(yNorth);
        area.setYSouth(ySouth);
        area.setXEast(xEast);
        area.setXWest(xWest);
        area.setLatNorth(latNorth);
        area.setLatSouth(latSouth);
        area.setLngEast(lngEast);
        area.setLngWest(lngWest);
        /*DEBUG("zoomLevel=%d yNorth=%d ySouth=%d xEast=%d xWest=%d latNorth=%f latSouth=%f lngEast=%f lngWest=%f",area.getZoomLevel(),
            area.getYNorth(),area.getYSouth(),area.getXEast(),area.getXWest(),
            area.getLatNorth(),area.getLatSouth(),area.getLngEast(),area.getLngWest());*/
        MapTile *preferredNeighbor=NULL;
        if (preferredNeighborCalibrationPath!="") {
          lockAccess(__FILE__,__LINE__);
          for (std::vector<MapContainer*>::iterator i=mapContainers.begin();i!=mapContainers.end();i++) {
            if ((*i)->getCalibrationFilePath()==preferredNeighborCalibrationPath) {
              if (preferredNeighbor) {
                FATAL("two map containers use same calibration path",NULL);
              }
              std::vector<MapTile*> *tiles=(*i)->getMapTiles();
              for (std::vector<MapTile*>::iterator j=tiles->begin();j!=tiles->end();j++) {
                if (((*j)->getMapX()==mapX)&&((*j)->getMapY()==mapY)) {
                  preferredNeighbor=*j;
                }
              }
            }
          }
          unlockAccess();
          if (!preferredNeighbor) {
            FATAL("remote side has a preferred neighbor that is not available at this side",NULL);
          }
        }
        if (preferredNeighbor)
          DEBUG("preferredNeighbor: %s,%d,%d",preferredNeighbor->getParentMapContainer()->getCalibrationFilePath(),preferredNeighbor->getMapX(),preferredNeighbor->getMapY());

        // Check if the navigation engine overlay hash is outdated
        queueRemoteNavigationEngineOverlayArchive(navigationEngineOverlayHash);

        // Search for the map tile
        lockAccess(__FILE__,__LINE__);
        std::list<MapTile*> tiles;
        fillGeographicAreaWithTiles(area,preferredNeighbor,maxTiles,&tiles);
        //DEBUG("tiles.size()=%d",tiles.size());
        for (std::list<MapTile*>::iterator i=tiles.begin();i!=tiles.end();i++) {
          MapTile *t=*i;
          //DEBUG("map container %s found",t->getParentMapContainer()->getCalibrationFilePath());
          queueRemoteMapContainer(t->getParentMapContainer(), &args, 20, &mapImagesToServe);
        }
        unlockAccess();
        commandProcessed=true;
      }

      // Shall we provide a map tile from a geo coordinate?
      if (cmdName=="findRemoteMapTileByGeographicCoordinate") {

        // Ensure that the map engine works and updates overlays (if screen is off)
        if ((core->getNavigationEngine()->mapGraphicUpdateIsRequired(__FILE__,__LINE__))) {
          core->getNavigationEngine()->updateMapGraphic();
        }

        // Convert the arguments
        double lng=atof(args[0].c_str());
        double lat=atof(args[1].c_str());
        double lngScale=atof(args[2].c_str());
        double latScale=atof(args[3].c_str());
        Int zoomLevel=atoi(args[4].c_str());
        bool lockZoomLevel=atoi(args[5].c_str());
        std::string preferredMapContainerCalibrationPath=args[6];
        std::string navigationEngineOverlayHash=args[7];
        MapPosition pos;
        pos.setLng(lng);
        pos.setLat(lat);
        pos.setLngScale(lngScale);
        pos.setLatScale(latScale);
        MapContainer *preferredMapContainer=NULL;
        if (preferredMapContainerCalibrationPath!="") {
          lockAccess(__FILE__,__LINE__);
          bool found=false;
          for (std::vector<MapContainer*>::iterator i=mapContainers.begin();i!=mapContainers.end();i++) {
            if ((*i)->getCalibrationFilePath()==preferredMapContainerCalibrationPath) {
              if (preferredMapContainer) {
                FATAL("two map containers use same calibration path",NULL);
              }
              preferredMapContainer=*i;
            }
          }
          unlockAccess();
          if (!preferredMapContainer) {
            FATAL("remote side has a preferred map container that is not available at this side",NULL);
          }
        }

        // Check if the navigation engine overlay hash is outdated
        queueRemoteNavigationEngineOverlayArchive(navigationEngineOverlayHash);

        // Search for the map tile
        lockAccess(__FILE__,__LINE__);
        MapTile *t = findMapTileByGeographicCoordinate(pos,zoomLevel,lockZoomLevel,preferredMapContainer);
        if (t) {
          //DEBUG("chosen map container: %s", t->getParentMapContainer()->getCalibrationFilePath());
          queueRemoteMapContainer(t->getParentMapContainer(), &args, 8, &mapImagesToServe);
        }
        unlockAccess();
        commandProcessed=true;
      }

      // Serve all extracted map images
      for (std::list<std::vector<std::string> >::iterator mapImage=mapImagesToServe.begin();mapImage!=mapImagesToServe.end();mapImage++) {

        // Get the required filenames
        std::string imageFilePath=(*mapImage)[0];
        std::string calibrationFilePath=(*mapImage)[1];
        std::string archiveFileFolder=(*mapImage)[2];
        std::string archiveFileName=(*mapImage)[3];

        // Only send this map container if not already sent
        bool alreadyServed=false;
        for (std::list<std::string>::iterator i=servedMapContainers.begin();i!=servedMapContainers.end();i++) {
          if (*i==calibrationFilePath) {
            //DEBUG("requested map container <%s> already served, skipping it", calibrationFilePath.c_str());
            alreadyServed=true;
          }
        }
        if (alreadyServed)
          continue;

        // Extract the needed files
        ZipArchive *mapArchive = new ZipArchive(archiveFileFolder,archiveFileName);
        if (!mapArchive) {
          FATAL("can not create zip archive",NULL);
          continue;
        }
        if (!mapArchive->init()) {
          delete mapArchive;
          WARNING("can not open <%s/%s>",archiveFileFolder.c_str(),archiveFileName.c_str());
          continue;
        }
        std::string imageExtension = imageFilePath.substr(imageFilePath.find_last_of(".")+1);
        std::string imageTempFilepath = workPath + "/remoteTile." + imageExtension;
        if (!mapArchive->exportEntry(imageFilePath,imageTempFilepath)) {
          delete mapArchive;
          WARNING("can not extract to file <%s>",imageTempFilepath.c_str());
          continue;
        }
        std::string calibrationExtension = calibrationFilePath.substr(calibrationFilePath.find_last_of(".")+1);
        std::string calibrationTempFilepath = workPath + "/remoteTile." + calibrationExtension;
        if (!mapArchive->exportEntry(calibrationFilePath,calibrationTempFilepath)) {
          delete mapArchive;
          WARNING("can not write to file <%s>",calibrationTempFilepath.c_str());
          continue;
        }
        delete mapArchive;

        // Create the remote tile archive
        std::stringstream remoteTileFilename;
        remoteTileFilename << "remoteTile" << tileNr << ".gda";
        tileNr++;
        mapArchive = new ZipArchive(workPath,remoteTileFilename.str());
        if (!mapArchive) {
          FATAL("can not create zip archive",NULL);
          continue;
        }
        mapArchive->init();
        if (!mapArchive->addEntry(imageFilePath,imageTempFilepath)) {
          delete mapArchive;
          WARNING("can not add entry to remote tile archive",NULL);
          continue;
        }
        if (!mapArchive->addEntry(calibrationFilePath,calibrationTempFilepath)) {
          delete mapArchive;
          WARNING("can not add entry to remote tile archive",NULL);
          continue;
        }
        if (!mapArchive->writeChanges()) {
          delete mapArchive;
          WARNING("can not write changes to remote tile archive",NULL);
          continue;
        }
        delete mapArchive;
        remove(imageTempFilepath.c_str());
        remove(calibrationTempFilepath.c_str());

        // Ask the app to transfer it to the remote side
        //DEBUG("sending %s to remote side",remoteTileFilename.str().c_str());
        std::string hash = Storage::computeMD5(workPath + "/" + remoteTileFilename.str());
        core->getCommander()->dispatch("serveRemoteMapArchive(" + workPath + "/" + remoteTileFilename.str() + "," + calibrationFilePath + "," + hash + ")");
        servedMapContainers.push_back(calibrationFilePath);
      }

      // Indicate issue if command has not been processed
      if (!commandProcessed) {
        WARNING("unknown command %s received",cmd.c_str());
      }
    }
  }
}

// Adds a new map archive
bool MapSource::addMapArchive(std::string path, std::string hash) {
  return false;
}

// Adds a new overlay archive
bool MapSource::addOverlayArchive(std::string path, std::string hash) {
  return false;
}

// Fills the given area with tiles
void MapSource::fillGeographicAreaWithTiles(MapArea area, MapTile *preferredNeighbor, Int maxTiles, std::list<MapTile*> *tiles) {

  // Check if the area is plausible
  if (area.getYNorth()<area.getYSouth()) {
    //DEBUG("north smaller than south",NULL);
    return;
  }
  if (area.getXEast()<area.getXWest()) {
    //DEBUG("east smaller than west",NULL);
    return;
  }

  // Check if the maximum number of tiles to display are reached
  if (tiles->size()>=maxTiles) {
    //DEBUG("too many tiles (maxTiles=%d)",maxTiles);
    return;
  }

  /* Visualize the search area
  if (core->getGraphicEngine()->getDebugMode()) {
    removeDebugPrimitives();
    GraphicRectangle *r;
    if (!(r=new GraphicRectangle())) {
      FATAL("no memory for graphic rectangle object",NULL);
      return;
    }
    //r->setColor(GraphicColor(((double)rand())/RAND_MAX*255,((double)rand())/RAND_MAX*255,((double)rand())/RAND_MAX*255));
    r->setColor(GraphicColor(255,0,0));
    r->setX(area.getXWest());
    r->setY(area.getYSouth());
    r->setWidth(area.getXEast()-area.getXWest());
    r->setHeight(area.getYNorth()-area.getYSouth());
    //r->setFilled(false);
    std::list<std::string> names;
    names.push_back("");
    //r->setName(names);
    r->setZ(-10);
    map->addPrimitive(r);
    core->interruptAllowedHere();
    DEBUG("redraw!",NULL);
  }*/

  //DEBUG("search area is (%d,%d)x(%d,%d)",area.getXWest(),area.getYNorth(),area.getXEast(),area.getYSouth());

  // Search for a tile that lies in the range
  MapContainer *container;
  MapTile *tile=findMapTileByGeographicArea(area,preferredNeighbor,container);
  //DEBUG("tile=0x%08x",tile);

  // Tile found?
  Int searchedYNorth,searchedYSouth;
  Int searchedXWest,searchedXEast;
  double searchedLatNorth,searchedLatSouth,searchedLngEast,searchedLngWest;
  Int visXByCalibrator,visYByCalibrator;
  if (tile) {

    // Remember the found tile
    tiles->push_back(tile);
    //DEBUG("tile <%s> found",tile->getParentMapContainer()->getCalibrationFilePath());

    // Use the calibrator to compute the position
    MapPosition pos=area.getRefPos();
    tile->getParentMapContainer()->getMapCalibrator()->setPictureCoordinates(pos);
    Int diffVisX=tile->getMapX()-pos.getX();
    Int diffVisY=tile->getMapY()-pos.getY();
    visXByCalibrator=area.getRefPos().getX()+diffVisX;
    visYByCalibrator=area.getRefPos().getY()-diffVisY-tile->getHeight();

    // If a tile has been found, reduce the search area by the found tile
    searchedYNorth=visYByCalibrator+tile->getHeight()-1;
    searchedYSouth=visYByCalibrator;
    searchedXWest=visXByCalibrator;
    searchedXEast=visXByCalibrator+tile->getWidth()-1;
    searchedLatNorth=tile->getLatNorthMin();
    searchedLatSouth=tile->getLatSouthMax();
    searchedLngWest=tile->getLngWestMax();
    searchedLngEast=tile->getLngEastMin();

  } else {

    //DEBUG("no tile found, stopping search",NULL);
    return;

  }

  //DEBUG("finished area is (%d,%d)x(%d,%d)",searchedXWest,searchedYNorth,searchedXEast,searchedYSouth);

  // Fill the new areas recursively if there are areas left
  MapArea nw=area;
  if (searchedYNorth>=area.getYSouth()) {
    nw.setYSouth(searchedYNorth+1);
    nw.setLatSouth(searchedLatNorth);
  }
  if (searchedXWest<=area.getXEast()) {
    nw.setXEast(searchedXWest-1);
    nw.setLngEast(searchedLngWest);
  }
  //DEBUG("search for new tile in north west quadrant",NULL);
  if (nw!=area)
    MapSource::fillGeographicAreaWithTiles(nw,tile,maxTiles,tiles);
  MapArea n=area;
  if (searchedYNorth>=area.getYSouth()) {
    n.setYSouth(searchedYNorth+1);
    n.setLatSouth(searchedLatNorth);
  }
  if (searchedXWest>area.getXWest()) {
    n.setXWest(searchedXWest);
    n.setLngWest(searchedLngWest);
  }
  if (searchedXEast<area.getXEast()) {
    n.setXEast(searchedXEast);
    n.setLngEast(searchedLngEast);
  }
  //DEBUG("search for new tile in north quadrant",NULL);
  if (n!=area)
    MapSource::fillGeographicAreaWithTiles(n,tile,maxTiles,tiles);
  MapArea ne=area;
  if (searchedYNorth>=area.getYSouth()) {
    ne.setYSouth(searchedYNorth+1);
    ne.setLatSouth(searchedLatNorth);
  }
  if (searchedXEast>=area.getXWest()) {
    ne.setXWest(searchedXEast+1);
    ne.setLngWest(searchedLngEast);
  }
  //DEBUG("search for new tile in north east quadrant",NULL);
  if (ne!=area)
    MapSource::fillGeographicAreaWithTiles(ne,tile,maxTiles,tiles);
  MapArea e=area;
  if (searchedYNorth<area.getYNorth()) {
    e.setYNorth(searchedYNorth);
    e.setLatNorth(searchedLatNorth);
  }
  if (searchedYSouth>area.getYSouth()) {
    e.setYSouth(searchedYSouth);
    e.setLatSouth(searchedLatSouth);
  }
  if (searchedXEast>=area.getXWest()) {
    e.setXWest(searchedXEast+1);
    e.setLngWest(searchedLngEast);
  }
  //DEBUG("search for new tile in east quadrant",NULL);
  if (e!=area)
    MapSource::fillGeographicAreaWithTiles(e,tile,maxTiles,tiles);
  MapArea se=area;
  if (searchedYSouth<=area.getYNorth()) {
    se.setYNorth(searchedYSouth-1);
    se.setLatNorth(searchedLatSouth);
  }
  if (searchedXEast>=area.getXWest()) {
    se.setXWest(searchedXEast+1);
    se.setLngWest(searchedLngEast);
  }
  //DEBUG("search for new tile in south east quadrant",NULL);
  if (se!=area)
    MapSource::fillGeographicAreaWithTiles(se,tile,maxTiles,tiles);
  MapArea s=area;
  if (searchedYSouth<=area.getYNorth()) {
    s.setYNorth(searchedYSouth-1);
    s.setLatNorth(searchedLatSouth);
  }
  if (searchedXWest>area.getXWest()) {
    s.setXWest(searchedXWest);
    s.setLngWest(searchedLngWest);
  }
  if (searchedXEast<area.getXEast()) {
    s.setXEast(searchedXEast);
    s.setLngEast(searchedLngEast);
  }
  //DEBUG("search for new tile in south quadrant",NULL);
  if (s!=area)
    MapSource::fillGeographicAreaWithTiles(s,tile,maxTiles,tiles);
  MapArea sw=area;
  if (searchedYSouth<=area.getYNorth()) {
    sw.setYNorth(searchedYSouth-1);
    sw.setLatNorth(searchedLatSouth);
  }
  if (searchedXWest<=area.getXEast()) {
    sw.setXEast(searchedXWest-1);
    sw.setLngEast(searchedLngWest);
  }
  //DEBUG("search for new tile in south west quadrant",NULL);
  if (sw!=area)
    MapSource::fillGeographicAreaWithTiles(sw,tile,maxTiles,tiles);
  MapArea w=area;
  if (searchedYNorth<area.getYNorth()) {
    w.setYNorth(searchedYNorth);
    w.setLatNorth(searchedLatNorth);
  }
  if (searchedYSouth>area.getYSouth()) {
    w.setYSouth(searchedYSouth);
    w.setLatSouth(searchedLatSouth);
  }
  if (searchedXWest<=area.getXEast()) {
    w.setXEast(searchedXWest-1);
    w.setLngEast(searchedLngWest);
  }
  //DEBUG("search for new tile in west quadrant",NULL);
  if (w!=area)
    MapSource::fillGeographicAreaWithTiles(w,tile,maxTiles,tiles);
}

// Creates all graphics
void MapSource::createGraphic() {
  if (isInitialized) {
    lockAccess(__FILE__,__LINE__);
    std::vector<MapContainer *> *maps=core->getMapSource()->getMapContainers();
    for (std::vector<MapContainer*>::const_iterator i=maps->begin();i!=maps->end();i++) {
      MapContainer *c=*i;
      std::vector<MapTile *> *tiles=c->getMapTiles();
      for (std::vector<MapTile*>::const_iterator j=(*tiles).begin();j!=(*tiles).end();j++) {
        MapTile *t=*j;
        t->createGraphic();
      }
    }
    unlockAccess();
  }
}

// Destroys all graphics
void MapSource::destroyGraphic() {
  if (isInitialized) {
    lockAccess(__FILE__,__LINE__);
    std::vector<MapContainer *> *maps=core->getMapSource()->getMapContainers();
    for (std::vector<MapContainer*>::const_iterator i=maps->begin();i!=maps->end();i++) {
      MapContainer *c=*i;
      std::vector<MapTile *> *tiles=c->getMapTiles();
      for (std::vector<MapTile*>::const_iterator j=(*tiles).begin();j!=(*tiles).end();j++) {
        MapTile *t=*j;
        t->destroyGraphic();
      }
    }
    unlockAccess();
  }
}

// Finds the path animator with the given name
GraphicPrimitive *MapSource::findPathAnimator(std::string name) {
  GraphicObject *pathAnimators=core->getDefaultGraphicEngine()->lockPathAnimators(__FILE__, __LINE__);
  GraphicPrimitiveMap *primitiveMap=pathAnimators->getPrimitiveMap();
  GraphicPrimitive *pathAnimator=NULL;
  for (GraphicPrimitiveMap::iterator i=primitiveMap->begin();i!=primitiveMap->end();i++) {
    GraphicPrimitive *p=i->second;
    if (p) {
      std::list<std::string> names = p->getName();
      if (names.size()>0) {
        if (names.front()==name) {
          pathAnimator=p;
          break;
        }
      }
    }
  }
  core->getDefaultGraphicEngine()->unlockPathAnimators();
  return pathAnimator;
}

// Finds the path animator with the given name
void MapSource::addPathAnimator(GraphicPrimitiveKey primitiveKey) {
  retrievedPathAnimators.push_back(primitiveKey);
}

}
