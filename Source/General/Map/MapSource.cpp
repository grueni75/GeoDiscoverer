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
  folder=core->getConfigStore()->getStringValue("Map","folder","Folder that contains calibrated maps");
  neighborDegreeTolerance=core->getConfigStore()->getDoubleValue("Map","neighborDegreeTolerance","// Maximum allowed difference in degrees to classify a tile as a neighbor",1e-7);
  mapTileLength=core->getConfigStore()->getIntValue("Map","tileLength","Width and height of a tile.",256);
  isInitialized=false;
}

MapSource::~MapSource() {
  deinit();
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
void MapSource::increaseProgress() {
  progressValue++;
  if (progressValue==progressUpdateValue) {
    core->getDialog()->updateProgress(progressDialog,progressDialogTitle,progressValue);
    progressIndex++;
    progressUpdateValue=progressIndex*progressValueMax/10;
  }
}

// Closes the progress bar
void MapSource::closeProgress() {
  core->getDialog()->closeProgress(progressDialog);
}

// Finds out which type of source to create
MapSourceTypes MapSource::determineType() {

  FILE *in;
  std::string folder=core->getConfigStore()->getStringValue("Map","folder","Folder that contains calibrated maps","Default");
  std::string infoPath = core->getHomePath() + "/Map/" + folder +"/info.gds";

  // If the info.gds file does not exist, it is an offline source
  if (!(in=fopen(infoPath.c_str(),"r"))) {
    return MapOfflineSourceType;
  } else {
    fclose(in);
    return MapOnlineSourceType;
  }
}

}
