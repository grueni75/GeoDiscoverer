//============================================================================
// Name        : MapContainer.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================


#include "Core.h"

namespace GEODISCOVERER {

// Constructor
MapContainer::MapContainer(bool doNotDelete) {

  // Set variables
  this->doNotDelete=doNotDelete;
  if (doNotDelete) {
    new(&this->mapTiles) std::vector<MapTile*>();
    new(&this->mapTilesIndexByMapTop) std::vector<Int>();
    new(&this->mapTilesIndexByMapBottom) std::vector<Int>();
    new(&this->mapTilesIndexByMapLeft) std::vector<Int>();
    new(&this->mapTilesIndexByMapRight) std::vector<Int>();
  } else {
    this->mapCalibrator=NULL;
    this->latNorth=-std::numeric_limits<double>::max();
    this->latSouth=+std::numeric_limits<double>::max();
    this->lngEast=-std::numeric_limits<double>::max();
    this->lngWest=+std::numeric_limits<double>::max();
    this->searchTree=NULL;
    this->leftChild=NULL;
    this->rightChild=NULL;
    this->downloadComplete=true;
    this->x=0;
    this->y=0;
    this->overlayGraphicInvalid=false;
    this->downloadRetries=0;
    this->mapFileFolder=NULL;
    this->imageFileName=NULL;
    this->imageFilePath=NULL;
    this->calibrationFileName=NULL;
    this->calibrationFilePath=NULL;
    this->archiveFileName=NULL;
    this->archiveFilePath=NULL;
    this->zoomLevelServer=0;
    this->zoomLevelMap=0;
    this->lngScale=0;
    this->latScale=0;
    this->imageType=ImageTypeUnknown;
    this->width=0;
    this->height=0;
  }
}

MapContainer::MapContainer(const MapContainer &pos) {
  FATAL("this object can not be copied",NULL);
}

// Destructor
MapContainer::~MapContainer() {
  if (mapCalibrator) MapCalibrator::destruct(mapCalibrator);
  for(std::vector<MapTile *>::const_iterator i=mapTiles.begin(); i != mapTiles.end(); i++) {
    MapTile::destruct(*i);
  }
  if (!doNotDelete) {
    if (mapFileFolder) free(mapFileFolder);
    if (imageFileName) free(imageFileName);
    if (imageFilePath) free(imageFilePath);
    if (calibrationFileName) free(calibrationFileName);
    if (calibrationFilePath) free(calibrationFilePath);
    if (archiveFileName) free(archiveFileName);
    if (archiveFilePath) free(archiveFilePath);
  }
}

// Operators
MapContainer &MapContainer::operator=(const MapContainer &rhs)
{
  FATAL("this object can not be copied",NULL);
  return *this;
}

// Adds a tile to the map
void MapContainer::addTile(MapTile *tile)
{
  Int index=mapTiles.size();
  mapTiles.push_back(tile);
  lngScale=tile->getLngScale();
  latScale=tile->getLatScale();
  if (tile->getMapY()==0) {
    if (tile->getLatNorthMax()>latNorth)
      latNorth=tile->getLatNorthMax();
  }
  if (tile->getMapY()==height-tile->getHeight()) {
    if (tile->getLatSouthMin()<latSouth)
      latSouth=tile->getLatSouthMin();
  }
  if (tile->getMapX()==0) {
    if (tile->getLngWestMin()<lngWest)
      lngWest=tile->getLngWestMin();
  }
  if (tile->getMapX()==width-tile->getWidth()) {
    if (tile->getLngEastMax()>lngEast)
      lngEast=tile->getLngEastMax();
  }

  // Add the new tile to the sorted index lists
  Int tileIndex=mapTiles.size()-1;
  insertTileToSortedList(&mapTilesIndexByMapTop,tile,tileIndex,PictureBorderTop);
  insertTileToSortedList(&mapTilesIndexByMapBottom,tile,tileIndex,PictureBorderBottom);
  insertTileToSortedList(&mapTilesIndexByMapRight,tile,tileIndex,PictureBorderRight);
  insertTileToSortedList(&mapTilesIndexByMapLeft,tile,tileIndex,PictureBorderLeft);
}

// Returns the map tile in which the position lies
MapTile *MapContainer::findMapTileByPictureCoordinate(MapPosition pos) {
  return findMapTileByPictureCoordinate(pos,searchTree,PictureBorderTop);
}

// Returns the map tile in which the position lies
MapTile *MapContainer::findMapTileByPictureCoordinate(MapPosition pos, MapTile *currentTile, PictureBorder currentDimension) {
  MapTile *bestTileLeft=NULL,*bestTileRight=NULL,*bestTile=NULL;
  PictureBorder nextDimension;
  bool useRightChild=true,useLeftChild=true;

  // Abort if tile is null
  if (!currentTile)
    return NULL;

  // Check that point lies within tile
  if ((pos.getX()>=currentTile->getMapX(0))&&
      (pos.getX()<=currentTile->getMapX(1))&&
      (pos.getY()>=currentTile->getMapY(0))&&
      (pos.getY()<=currentTile->getMapY(1))) {
    //DEBUG("picture coordinate lies within tile 0x%08x => tile found",currentTile);
    return currentTile;
  }

  // Decide with which branches of the tree to continue
  switch(currentDimension) {
    case PictureBorderTop:
      nextDimension=PictureBorderBottom;
      if (currentTile->getMapY(0)>pos.getY()) {
        useRightChild=false;
      }
      break;
    case PictureBorderBottom:
      nextDimension=PictureBorderRight;
      if (currentTile->getMapY(1)<pos.getY()) {
        useLeftChild=false;
      }
      break;
    case PictureBorderRight:
      nextDimension=PictureBorderLeft;
      if (currentTile->getMapX(1)<pos.getX()) {
        useLeftChild=false;
      }
      break;
    case PictureBorderLeft:
      nextDimension=PictureBorderTop;
      if (currentTile->getMapX(0)>pos.getX()) {
        useRightChild=false;
      }
      break;
  }

  // Get the best tiles from the left and the right branch
  if (useRightChild) {
    //DEBUG("search for better matching tile in right branch",NULL);
    bestTileRight=findMapTileByPictureCoordinate(pos,currentTile->getRightChild(),nextDimension);
  }
  if ((useLeftChild)&&(!bestTileRight)) {
    //DEBUG("search for better matching tile in left branch",NULL);
    bestTileLeft=findMapTileByPictureCoordinate(pos,currentTile->getLeftChild(),nextDimension);
  }
  if (bestTileRight)
    bestTile=bestTileRight;
  if (bestTileLeft)
    bestTile=bestTileLeft;
  //DEBUG("found tile = 0x%08x",bestTile);
  return bestTile;

}

// Returns the map tile that lies in a given area and that is closest to the given neigbor
MapTile *MapContainer::findMapTileByPictureArea(MapArea area, MapTile *preferredNeigbor) {
  double bestDistance=+std::numeric_limits<double>::max();
  bool betterTileFound=false;
  MapTile *foundTile=findMapTileByPictureArea(area,preferredNeigbor,searchTree,PictureBorderTop,bestDistance,betterTileFound);
  //DEBUG("found tile = %08x",foundTile);
  return foundTile;
}

// Returns the map tile that lies in a given area and that is closest to the given neigbor
MapTile *MapContainer::findMapTileByPictureArea(MapArea area, MapTile *preferredNeigbor, MapTile *currentTile, PictureBorder currentDimension, double &bestDistance, bool &betterTileFound, std::list<MapTile*> *foundMapTiles) {

  MapTile *bestTileLeft=NULL,*bestTileRight=NULL;
  double bestDistanceLeft, bestDistanceRight;
  PictureBorder nextDimension;
  bool useRightChild=true,useLeftChild=true;
  MapTile *bestTile=NULL;
  bool betterTileFoundLeft=false,betterTileFoundRight=false;

  // Set variables
  betterTileFound=false;

  // Abort if tile is null
  if (!currentTile)
    return NULL;

  // Search for the map tile that covers parts of the given area
  // if preferredNeighbor=NULL:
  //   Use the first matching tile
  // if preferredNeighbor!=NULL:
  //   Use the closest matching tile

  // Check that the tile overlaps the area
  if ((currentTile->getMapX(0)<area.getXEast())&&(currentTile->getMapX(1)>area.getXWest())&&
      (currentTile->getMapY(0)<area.getYSouth())&&(currentTile->getMapY(1)>area.getYNorth())) {

    // Shall we return all matching tiles?
    if (foundMapTiles) {

      foundMapTiles->push_back(currentTile);

    } else {

      // Preferred neighbor given?
      if (!preferredNeigbor) {

        // No preferred neighbor, use the first one
        //DEBUG("current tile overlaps area and no preferred neigbor => tile found",NULL);
        betterTileFound=true;
        return currentTile;

      } else {

        // Compute the distance to the preferred neighbor
        Int diffX=currentTile->getMapCenterX()-preferredNeigbor->getMapCenterX();
        Int diffY=currentTile->getMapCenterY()-preferredNeigbor->getMapCenterY();
        Int distance=diffX*diffX+diffY*diffY;
        //DEBUG("map tile name: %20s distance: %d",t->getVisName().c_str(),distance);

        // Update the best tile if this one lies closer
        if (distance<bestDistance) {
          //DEBUG("current tile overlaps area and lies closer to preferred neigbor => tile is current best candidate",NULL);
          bestDistance=distance;
          bestTile=currentTile;
          betterTileFound=true;
        }

      }

    }

  }

  // Decide with which branches of the tree to continue
  switch(currentDimension) {
    case PictureBorderTop:
      nextDimension=PictureBorderBottom;
      if (currentTile->getMapY(0)>=area.getYSouth()) {
        useRightChild=false;
      }
      break;
    case PictureBorderBottom:
      nextDimension=PictureBorderRight;
      if (currentTile->getMapY(1)<=area.getYNorth()) {
        useLeftChild=false;
      }
      break;
    case PictureBorderRight:
      nextDimension=PictureBorderLeft;
      if (currentTile->getMapX(1)<=area.getXWest()) {
        useLeftChild=false;
      }
      break;
    case PictureBorderLeft:
      nextDimension=PictureBorderTop;
      if (currentTile->getMapX(0)>=area.getXEast()) {
        useRightChild=false;
      }
      break;
  }

  // Get the best tiles from the left and the right branch
  bestDistanceRight=bestDistance;
  if (useRightChild) {
    //DEBUG("search for better matching tile in right branch",NULL);
    bestTileRight=findMapTileByPictureArea(area,preferredNeigbor,currentTile->getRightChild(),nextDimension,bestDistanceRight,betterTileFoundRight,foundMapTiles);
  }
  bestDistanceLeft=bestDistanceRight;
  if ((useLeftChild)&&(preferredNeigbor||!bestTileRight)) {
    //DEBUG("search for better matching tile in left branch",NULL);
    bestTileLeft=findMapTileByPictureArea(area,preferredNeigbor,currentTile->getLeftChild(),nextDimension,bestDistanceLeft,betterTileFoundLeft,foundMapTiles);
  }
  if (betterTileFoundLeft) {
    bestDistance=bestDistanceLeft;
    betterTileFound=true;
    return bestTileLeft;
  }
  if (betterTileFoundRight) {
    bestDistance=bestDistanceRight;
    betterTileFound=true;
    return bestTileRight;
  }
  return bestTile;
}

// Returns all map tiles that lies in a given area
std::list<MapTile*> MapContainer::findMapTilesByPictureArea(MapArea area) {
  double bestDistance=+std::numeric_limits<double>::max();
  bool betterTileFound=false;
  std::list<MapTile*> result;
  findMapTileByPictureArea(area,NULL,searchTree,PictureBorderTop,bestDistance,betterTileFound,&result);
  return result;
}

// Gets the next field in a semicolon seperated list
std::string MapContainer::getNextSemicolonField(std::string list, Int &start) {
  std::string field;
  Int end=list.substr(start).find_first_of(";")+start;
  if (end==std::string::npos)
    end=list.size()-1;
  field=list.substr(start,end-start);
  //DEBUG("start=%d end=%d field=%s",start,end,field.c_str());
  start=end+1;
  return field;
}

/* Reads a gmi file
bool MapContainer::readGMICalibrationFile()
{
  std::string line;
  Int imageWidth;
  Int imageHeight;
  std::ifstream in;
  std::string t;
  Int i,j,k;
  double d;
  enum { gmiHeader, gmiFilename, gmiWidth, gmiHeight, gmiCalibrationPoint, gmiFinished } parserState;
  std::istringstream iss;

  // Open the map calibration file
  in.open (calibrationFilePath.c_str());
  if (!in.is_open()) {
    ERROR("can not open file <%s> for reading map calibration",calibrationFilePath.c_str());
    return false;
  }

  // Create a new calibrator object
  mapCalibrator=new MapCalibrator();
  if (!mapCalibrator) {
    FATAL("can not create map calibrator",NULL);
    return false;
  }

  // Parse it contents
  parserState=gmiHeader;
  while(!in.eof() && parserState!=gmiFinished) {
    getline(in,line);
    while ((line.substr(line.size()-1)=="\n")||(line.substr(line.size()-1)=="\r"))
      line=line.substr(0,line.size()-1);
    switch(parserState) {
      case gmiHeader:
        if (line.substr(0,30)!="Map Calibration data file v3.0") {
          ERROR("file format of <%s> is not supported (header incorrect)",calibrationFilePath.c_str());
          return false;
        }
        parserState=gmiFilename;
        break;
      case gmiFilename:
        imageFileName=line;
        imageFilePath=mapFileFolder+"/"+imageFileName;
        parserState=gmiWidth;
        break;
      case gmiWidth:
        iss.str(line);
        iss.clear();
        iss >> imageWidth;
        parserState=gmiHeight;
        break;
      case gmiHeight:
        iss.str(line);
        iss.clear();
        iss >> imageHeight;
        parserState=gmiCalibrationPoint;
        break;
      case gmiCalibrationPoint:
        if (line=="Border and Scale") {
          parserState=gmiFinished;
        } else {
          MapPosition pos;
          i=0;
          iss.str(getNextSemicolonField(line,i)); iss.clear(); iss >> k; pos.setX(k);
          iss.str(getNextSemicolonField(line,i)); iss.clear(); iss >> k; pos.setY(k);
          iss.str(getNextSemicolonField(line,i)); iss.clear(); iss >> d; pos.setLng(d);
          iss.str(getNextSemicolonField(line,i)); iss.clear(); iss >> d; pos.setLat(d);
          mapCalibrator->addCalibrationPoint(pos);
        }
        break;
      default:
        FATAL("unknown parser state",NULL);
        return false;
    }
  }
  in.close();
  if (parserState!=gmiFinished) {
    ERROR("parsing of <%s> could not be finished (file corrupt?)",calibrationFilePath.c_str());
    return false;
  }

  return true;
}*/

// Reads a calibration file
bool MapContainer::readCalibrationFile(std::string fileFolder, std::string fileBasename, std::string fileExtension)
{
  MapTile *mapTile=NULL;
  Int tileWidth=core->getMapSource()->getMapTileWidth();
  Int tileHeight=core->getMapSource()->getMapTileHeight();
  bool calibrated=false;
  Int imageWidth;
  Int imageHeight;

  // Set some variables
  setMapFileFolder(fileFolder);
  setCalibrationFileName(fileBasename + "." + fileExtension);

  /* GPS Tuner file format?
  if (fileExtension=="gmi") {
    if (!readGMICalibrationFile())
      return false;
    calibrated=true;
  }*/

  // Geo Discoverer Map?
  if (fileExtension=="gdm") {
    if (!readGDMCalibrationFile())
      return false;
    calibrated=true;
  }

  // Calibration not supported?
  if (!calibrated) {
    ERROR("calibration file extension <%s> is not supported",fileExtension.c_str());
    return false;
  }

  // Check if we have enough calibration points
  if (mapCalibrator->numberOfCalibrationPoints()<3) {
    ERROR("at least three calibration points must be specified in <%s>",calibrationFilePath);
    return false;
  }

  // Extract the image
  std::string tempImageFilePath = core->getMapSource()->getFolderPath() + "/tile.bin";
  std::list<ZipArchive*> *mapArchives = core->getMapSource()->lockMapArchives(__FILE__, __LINE__);
  for (std::list<ZipArchive*>::iterator i=mapArchives->begin();i!=mapArchives->end();i++) {
    if ((*i)->getEntrySize(imageFilePath)>0) {
      (*i)->exportEntry(imageFilePath,tempImageFilePath);
    }
  }
  core->getMapSource()->unlockMapArchives();

  // Check the type and dimension of the image
  if (core->getImage()->queryPNG(tempImageFilePath,imageWidth,imageHeight)) {
    imageType=ImageTypePNG;
  } else if (core->getImage()->queryJPEG(tempImageFilePath,imageWidth,imageHeight)) {
    imageType=ImageTypeJPEG;
  } else {
    ERROR("file format of image <%s> not supported",imageFilePath);
    return false;
  }

  // Find out how many number of tiles are required to hold this image
  Int tileCountX=imageWidth/tileWidth;
  if (imageWidth%tileWidth!=0)
    WARNING("some part of the right border of image <%s> is not used because image width is not a multiple of <%d>",imageFileName,tileWidth);
  Int tileCountY=imageHeight/tileHeight;
  if (imageHeight%tileHeight!=0)
    WARNING("some part of the bottom border of image <%s> is not used because image height is not a multiple of <%d>",imageFileName,tileHeight);
  if (tileCountX==0) {
    ERROR("width of image <%s> is too small (must be at least %d but is %d)",imageFileName,tileWidth,imageWidth);
    return false;
  }
  if (tileCountY==0) {
    ERROR("height of image <%s> is too small (must be at least %d but is %d)",imageFileName,tileHeight,imageHeight);
    return false;
  }

  // Update some variables
  width=tileCountX*tileWidth;
  height=tileCountY*tileHeight;

  // Create the map tiles for this image
  for (Int x=0;x<tileCountX;x++) {
    for (Int y=0;y<tileCountY;y++) {

      // Create the new tile
      MapTile *tile=new MapTile(x*tileWidth,y*tileHeight,this);
      //DEBUG("tile borders at position (%d,%d): %f %f %f %f",y,x,tile->getLatSouth(),tile->getLatNorth(),tile->getLngWest(),tile->getLngEast());
      if (!tile) {
        FATAL("can not create map tile",NULL);
        return false;
      }
      addTile(tile);

    }
  }

  // Update the search structures
  createSearchTree();

  // Create a new calibration file if the used one was not the native one
  if (fileExtension!="gdm") {
    std::string t=fileBasename + "." + "gdm";
    t=std::string(mapFileFolder) + "/" + t;
    setCalibrationFileName(t);
    writeCalibrationFile(NULL);
  }

  // Success!
  return true;
}

// Creates the search tree
void MapContainer::createSearchTree() {
  createSearchTree(NULL,false,PictureBorderTop,mapTilesIndexByMapTop);
}

// Returns the file extension of supported calibration files
std::string MapContainer::getCalibrationFileExtension(Int i)
{
  if (i==0) {
    return "gdm";
  }
  if (i==1) {
    return "gmi";
  }
  return "";
}

// Checks if the extension is supported as a calibration file
bool MapContainer::calibrationFileIsSupported(std::string extension)
{
  std::string supportedExtension="-";
  for (Int i=0;supportedExtension!="";i++) {
    supportedExtension=getCalibrationFileExtension(i);
    if (supportedExtension==extension)
      return true;
  }
  return false;
}

// Sorts the list associated with the given border
void MapContainer::insertTileToSortedList(std::vector<Int> *list, MapTile *newTile, Int newTileIndex, PictureBorder border) {
  std::vector<Int>::iterator i;
  bool inserted=false;
  for (i=list->begin();i<list->end();i++) {
    if (mapTiles[*i]->getMapBorder(border) > newTile->getMapBorder(border)) {
      list->insert(i,newTileIndex);
      inserted=true;
      break;
    }
  }
  if (!inserted)
    list->push_back(newTileIndex);
}

// Creates a sorted index vector from a given sorted index vector masked by range on an other index vector
void MapContainer::createMaskedIndexVector(std::vector<Int> *currentIndexVector, std::vector<Int> *allowedIndexVector, std::vector<Int> &sortedIndexVector) {
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
void MapContainer::createSearchTree(MapTile *parentNode, bool leftBranch, PictureBorder dimension, std::vector<Int> remainingMapTilesIndex) {

  PictureBorder nextDimension;
  Int medianIndex;
  MapTile *rootNode,*tempNode;
  bool lessEqual=false;
  bool greaterEqual=false;
  std::vector<Int> sortedVector;

  // Update variables depending on the dimension
  switch(dimension) {
    case PictureBorderTop:
      nextDimension=PictureBorderBottom;
      createMaskedIndexVector(&mapTilesIndexByMapTop,&remainingMapTilesIndex,sortedVector);
      greaterEqual=true;
      break;
    case PictureBorderBottom:
      nextDimension=PictureBorderRight;
      createMaskedIndexVector(&mapTilesIndexByMapBottom,&remainingMapTilesIndex,sortedVector);
      lessEqual=true;
      break;
    case PictureBorderRight:
      nextDimension=PictureBorderLeft;
      createMaskedIndexVector(&mapTilesIndexByMapRight,&remainingMapTilesIndex,sortedVector);
      lessEqual=true;
      break;
    case PictureBorderLeft:
      nextDimension=PictureBorderTop;
      createMaskedIndexVector(&mapTilesIndexByMapLeft,&remainingMapTilesIndex,sortedVector);
      greaterEqual=true;
      break;
  }

  // Compute median index
  medianIndex=sortedVector.size()/2;
  if (lessEqual) {
    while ((medianIndex<sortedVector.size()-1)&&(mapTiles[sortedVector[medianIndex]]->getMapBorder(dimension)==mapTiles[sortedVector[medianIndex+1]]->getMapBorder(dimension)))
      medianIndex++;
  }
  if (greaterEqual) {
    while ((medianIndex>0)&&(mapTiles[(sortedVector)[medianIndex]]->getMapBorder(dimension)==mapTiles[(sortedVector)[medianIndex-1]]->getMapBorder(dimension)))
      medianIndex--;
  }

  // Add node to the tree
  rootNode=mapTiles[(sortedVector)[medianIndex]];
  if (!parentNode) {
    searchTree=rootNode;
  } else {
    if (leftBranch) {
      parentNode->setLeftChild(rootNode);
    } else {
      parentNode->setRightChild(rootNode);
    }
  }

  // Recursively process the remaining nodes
  std::vector<Int>::iterator medianIterator=sortedVector.begin()+medianIndex;
  if (0!=medianIndex) {
    std::vector<Int> t(sortedVector.begin(),medianIterator);
    createSearchTree(rootNode,true,nextDimension,t);
  } else {
    rootNode->setLeftChild(NULL);
  }
  if ((sortedVector.size()-1)!=medianIndex) {
    std::vector<Int> t(medianIterator+1,sortedVector.end());
    createSearchTree(rootNode,false,nextDimension,t);
  } else {
    rootNode->setRightChild(NULL);
  }
}

// Returns the geographic coordinate of the given border
double MapContainer::getBorder(GeographicBorder border)
{
  switch(border) {
    case GeographicBorderLatNorth:
      return getLatNorth();
      break;
    case GeographicBorderLatSouth:
      return getLatSouth();
      break;
    case GeographicBorderLngEast:
      return getLngEast();
      break;
    case GeographicBorderLngWest:
      return getLngWest();
      break;
    default:
      FATAL("unsupported border",NULL);
      break;
  }
  return 0;
}

// Stores the contents of the search tree in a binary file
void MapContainer::storeSearchTree(std::ofstream *ofs, MapTile *node) {

  // Write the contents of the node
  node->store(ofs);

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
void MapContainer::store(std::ofstream *ofs) {

  // Write the size of the object for detecting changes later
  Int size=sizeof(*this);
  Storage::storeInt(ofs,size);

  // Store all relevant fields
  MapContainer *leftChild=this->leftChild;
  MapContainer *rightChild=this->rightChild;
  MapTile *searchTree=this->searchTree;
  this->leftChild=NULL;
  this->rightChild=NULL;
  this->searchTree=NULL;
  Storage::storeMem(ofs,(char*)this,sizeof(MapContainer),true);
  this->leftChild=leftChild;
  this->rightChild=rightChild;
  this->searchTree=searchTree;
  Storage::storeString(ofs,mapFileFolder);
  Storage::storeString(ofs,imageFileName);
  Storage::storeString(ofs,imageFilePath);
  Storage::storeString(ofs,calibrationFileName);
  Storage::storeString(ofs,calibrationFilePath);
  Storage::storeString(ofs,archiveFileName);
  Storage::storeString(ofs,archiveFilePath);
  mapCalibrator->store(ofs);

  // Store the search tree
  Storage::storeInt(ofs,mapTiles.size());
  if (searchTree) {
    Storage::storeBool(ofs,true);
    storeSearchTree(ofs,searchTree);
  } else {
    Storage::storeBool(ofs,false);
  }

}

// Reads the contents of the search tree from a binary file
MapTile *MapContainer::retrieveSearchTree(MapContainer *mapContainer, Int &nodeNumber, char *&cacheData, Int &cacheSize, bool &abortRetrieve) {

  // Read the current node
  MapTile *node=MapTile::retrieve(cacheData,cacheSize,mapContainer);
  if (node==NULL) {
    abortRetrieve=true;
    return NULL;
  }
  mapContainer->mapTiles[nodeNumber]=node;
  nodeNumber++;

  // Update the progress
  if (!core->getMapSource()->increaseProgress()) {
    DEBUG("aborting search tree retrieve because core quit is requested",NULL);
    return node;
  }

  // Read the left node
  bool hasLeftNode;
  Storage::retrieveBool(cacheData,cacheSize,hasLeftNode);
  if (hasLeftNode) {
    node->setLeftChild(retrieveSearchTree(mapContainer,nodeNumber,cacheData,cacheSize,abortRetrieve));
    if (abortRetrieve)
      return NULL;
  }

  // Read the right node
  bool hasRightNode;
  Storage::retrieveBool(cacheData,cacheSize,hasRightNode);
  if (hasRightNode) {
    node->setRightChild(retrieveSearchTree(mapContainer,nodeNumber,cacheData,cacheSize,abortRetrieve));
    if (abortRetrieve)
      return NULL;
  }

  // Return the node
  return node;
}

// Reads the contents of the object from a binary file
MapContainer *MapContainer::retrieve(char *&cacheData, Int &cacheSize) {

  //PROFILE_START;

  // Check if the class has changed
  Int size=sizeof(MapContainer);
#ifdef TARGET_LINUX
  if (size!=320) {
    FATAL("unknown size of object (%d), please adapt class storage",size);
    return NULL;
  }
#endif

  // Read the size of the object and check with current size
  size=0;
  Storage::retrieveInt(cacheData,cacheSize,size);
  if (size!=sizeof(MapContainer)) {
    DEBUG("stored size of object does not match implemented object size, aborting retrieve",NULL);
    return NULL;
  }
  //PROFILE_ADD("sanity check");

  // Create a new map container object
  //PROFILE_ADD("map container init");
  Storage::skipPadding(cacheData,cacheSize);
  cacheSize-=sizeof(MapContainer);
  if (cacheSize<0) {
    DEBUG("can not create map container object",NULL);
    return NULL;
  }
  MapContainer *mapContainer=(MapContainer*)cacheData;
  mapContainer=new(cacheData) MapContainer(true);
  cacheData+=sizeof(MapContainer);
  //PROFILE_ADD("object creation");

  // Read the fields
  Storage::retrieveString(cacheData,cacheSize,&mapContainer->mapFileFolder);
  Storage::retrieveString(cacheData,cacheSize,&mapContainer->imageFileName);
  Storage::retrieveString(cacheData,cacheSize,&mapContainer->imageFilePath);
  Storage::retrieveString(cacheData,cacheSize,&mapContainer->calibrationFileName);
  Storage::retrieveString(cacheData,cacheSize,&mapContainer->calibrationFilePath);
  Storage::retrieveString(cacheData,cacheSize,&mapContainer->archiveFileName);
  Storage::retrieveString(cacheData,cacheSize,&mapContainer->archiveFilePath);

  //PROFILE_ADD("field read"");
  mapContainer->mapCalibrator=MapCalibrator::retrieve(cacheData,cacheSize);
  if (mapContainer->mapCalibrator==NULL) {
    MapContainer::destruct(mapContainer);
    return NULL;
  }
  //PROFILE_ADD("map calibrator retrieve");

  // Read the search tree
  Storage::retrieveInt(cacheData,cacheSize,size);
  mapContainer->mapTiles.resize(size);
  bool hasSearchTree;
  Storage::retrieveBool(cacheData,cacheSize,hasSearchTree);
  if (hasSearchTree) {
    Int nodeNumber=0;
    bool abortRetrieve=false;
    mapContainer->searchTree=retrieveSearchTree(mapContainer,nodeNumber,cacheData,cacheSize,abortRetrieve);
    if ((mapContainer->searchTree==NULL)||(abortRetrieve)||(core->getQuitCore())) {
      mapContainer->mapTiles.resize(nodeNumber);
      MapContainer::destruct(mapContainer);
      return NULL;
    }
  }
  //PROFILE_ADD("map tile retrieve");

  // Return result
  return mapContainer;
}

// Destructs the objects correctly (i.e., if memory has not been allocated by new)
void MapContainer::destruct(MapContainer *object) {
  if (object->doNotDelete) {
    object->~MapContainer();
  } else {
    delete object;
  }
}

// Checks if the container contains tiles that are currently used for screen drawing
bool MapContainer::isDrawn() {
  for(std::vector<MapTile*>::iterator i=mapTiles.begin();i!=mapTiles.end();i++) {
    if ((*i)->isDrawn())
      return true;
  }
  return false;
}

// Finds out the last access time
TimestampInSeconds MapContainer::getLastAccess() {
  TimestampInSeconds newestAccess=0;
  for(std::vector<MapTile*>::iterator i=mapTiles.begin();i!=mapTiles.end();i++) {
    if ((*i)->getLastAccess()>newestAccess) {
      newestAccess=(*i)->getLastAccess();
    }
  }
  return newestAccess;
}

}



