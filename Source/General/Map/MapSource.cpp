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

MapSource::MapSource() {
  folder=core->getConfigStore()->getStringValue("Map","folder");
  neighborPixelTolerance=core->getConfigStore()->getDoubleValue("Map","neighborPixelTolerance");
  mapTileLength=core->getConfigStore()->getIntValue("Map","tileLength");
  statusMutex=core->getThread()->createMutex("map source status mutex");
  isInitialized=false;
  contentsChanged=false;
  centerPosition=NULL;
  mapArchivesMutex=core->getThread()->createMutex("map archives mutex");
}

MapSource::~MapSource() {
  deinit();
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

// Creates a new map source object of the correct type
MapSource *MapSource::newMapSource() {

  std::string infoPath = core->getHomePath() + "/Map/" + core->getConfigStore()->getStringValue("Map","folder") + "/info.gds";

  // If the info.gds file does not exist, it is an offline source
  if (access(infoPath.c_str(),F_OK)) {
    return new MapSourceCalibratedPictures();
  } else {
    return new MapSourceMercatorTiles();
  }
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
std::list<MapContainer*> MapSource::findMapContainersByGeographicCoordinate(MapPosition pos) {

  bool betterMapContainerFound=false;
  MapPosition bestPos;
  double distToNearestLngScale=-1, distToNearestLatScale=-1;
  std::list<MapContainer*> result;

  // Do not work if the source has no search trees
  if (zoomLevelSearchTrees.size()==0)
    return result;

  // Get the search tree to use
  MapContainerTreeNode *startNode=zoomLevelSearchTrees[0];

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
    Int zoomLevel=mapContainer->getZoomLevel();
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


}
