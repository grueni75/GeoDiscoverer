//============================================================================
// Name        : MapTile.cpp
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

MapTile::MapTile(Int mapX, Int mapY, MapContainer *parent, bool doNotInit, bool doNotDelete) {

  //PROFILE_START

  // Read config
  this->width=core->getMapSource()->getMapTileWidth();
  this->height=core->getMapSource()->getMapTileHeight();
  //PROFILE_ADD("config read")

  // Copy variables
  this->doNotDelete=doNotDelete;
  this->mapX[0]=mapX;
  this->mapY[0]=mapY;
  this->parent=parent;
  this->isHidden=false;
  //DEBUG("width=%d height=%d",width,height);
  //PROFILE_ADD("basic variable copy")
  this->rectangle.setWidth(width);
  this->rectangle.setHeight(height);
  //DEBUG("w=%d h=%d",rectangle->getWidth(),rectangle->getHeight());
  //PROFILE_ADD("rectangle variable copy")

  // Set dependent variables
  this->distance=0;
  this->isProcessed=false;
  this->leftChild=NULL;
  this->rightChild=NULL;
  isCached=false;
  rectangle.setColor(GraphicColor(255,255,255,255));
  this->mapX[1]=this->mapX[0]+this->width-1;
  this->mapY[1]=this->mapY[0]+this->height-1;
  this->isDummy=false;
  this->lastAccess=0;
  std::list<std::string> name;
  std::stringstream s;
  name.push_back(parent->getImageFileName());
  s << this->mapX[0] << "," << this->mapY[0] << " - ";
  s << this->mapX[1] << "," << this->mapY[1];
  name.push_back(s.str());
  rectangle.setName(name);
  //PROFILE_ADD("dependent variable computation")
  visualization.addPrimitive(&rectangle);
  this->visualizationKey=0;
  this->visX=visualization.getX();
  this->visY=visualization.getY();
  this->visZ=visualization.getZ();

  // Init the remaining object
  if (!doNotInit)
    init();
}

MapTile::~MapTile() {
  visualization.lockAccess();
  if (visualization.getPrimitiveMap()->size()!=1) {
    FATAL("expected only the rectangle in the visualization object",NULL);
  }
  visualization.unlockAccess();
}

// Does the computationly intensive part of the initialization
void MapTile::init() {

  // Compute borders
  MapPosition pos,upperLeftCorner,upperRightCorner,lowerLeftCorner,lowerRightCorner;
  MapCalibrator *calibrator=parent->getMapCalibrator();
  pos.setX(this->mapX[0]);
  pos.setY(this->mapY[0]);
  calibrator->setGeographicCoordinates(pos);
  latY[0]=pos.getLat();
  lngX[0]=pos.getLng();
  upperLeftCorner=pos;
  pos.setX(this->mapX[1]+1);
  calibrator->setGeographicCoordinates(pos);
  lngX[1]=pos.getLng();
  upperRightCorner=pos;
  pos.setY(this->mapY[1]+1);
  calibrator->setGeographicCoordinates(pos);
  latY[1]=pos.getLat();
  lowerRightCorner=pos;
  pos.setX(this->mapX[0]);
  calibrator->setGeographicCoordinates(pos);
  lowerLeftCorner=pos;
  if (upperLeftCorner.getLat()>upperRightCorner.getLat()) {
    this->latNorthMax=upperLeftCorner.getLat();
    this->latNorthMin=upperRightCorner.getLat();
  } else {
    this->latNorthMax=upperRightCorner.getLat();
    this->latNorthMin=upperLeftCorner.getLat();
  }
  if (lowerLeftCorner.getLat()<lowerRightCorner.getLat()) {
    this->latSouthMin=lowerLeftCorner.getLat();
    this->latSouthMax=lowerRightCorner.getLat();
  } else {
    this->latSouthMin=lowerRightCorner.getLat();
    this->latSouthMax=lowerLeftCorner.getLat();
  }
  if (upperLeftCorner.getLng()<lowerLeftCorner.getLng()) {
    this->lngWestMin=upperLeftCorner.getLng();
    this->lngWestMax=lowerLeftCorner.getLng();
  } else {
    this->lngWestMin=lowerLeftCorner.getLng();
    this->lngWestMax=upperLeftCorner.getLng();
  }
  if (upperRightCorner.getLng()>lowerRightCorner.getLng()) {
    this->lngEastMax=upperRightCorner.getLng();
    this->lngEastMin=lowerRightCorner.getLng();
  } else {
    this->lngEastMax=lowerRightCorner.getLng();
    this->lngEastMin=upperRightCorner.getLng();
  }

  // Compute the orientation of the map with respect to true north
  MapPosition middleLowerPoint, middleUpperPoint;
  middleLowerPoint.setLng(lngWestMin+(lngEastMax-lngWestMin)/2);
  middleLowerPoint.setLat(latSouthMin);
  calibrator->setPictureCoordinates(middleLowerPoint);
  middleUpperPoint=middleLowerPoint;
  middleUpperPoint.setLat(latNorthMax);
  calibrator->setPictureCoordinates(middleUpperPoint);
  Int deltaX=middleUpperPoint.getX()-middleLowerPoint.getX();
  Int deltaY=middleLowerPoint.getY()-middleUpperPoint.getY();
  northAngle=FloatingPoint::rad2degree(FloatingPoint::computeAngle(deltaY,deltaX));

  // Compute the scale;
  this->lngScale=((double)(this->mapX[1]-this->mapX[0]))/fabs(lngEastMax-lngWestMin);
  this->latScale=((double)(this->mapY[1]-this->mapY[0]))/fabs(latNorthMax-latSouthMin);
  //DEBUG("%s: lngScale=%d latScale=%d",parent->getPictureFilename().c_str(),lngScale,latScale);
}

// Returns a map position object of the center
MapPosition MapTile::getMapCenterPosition() {
  MapPosition pos;
  pos.setLat(getLatCenter());
  pos.setLatScale(latScale);
  pos.setLng(getLngCenter());
  pos.setLngScale(lngScale);
  pos.setMapTile(this);
  pos.setX(getMapCenterX());
  pos.setY(getMapCenterY());
  return pos;
}

// Returns the picture coordinate of the given border
Int MapTile::getMapBorder(PictureBorder border)
{
  switch(border) {
    case PictureBorderTop:
      return getMapY(0);
      break;
    case PictureBorderBottom:
      return getMapY(1);
      break;
    case PictureBorderLeft:
      return getMapX(0);
      break;
    case PictureBorderRight:
      return getMapX(1);
      break;
    default:
      FATAL("unsupported border",NULL);
      break;
  }
  return 0;
}

// Store the contents of the object in a binary file
void MapTile::store(std::ofstream *ofs, Int &memorySize) {

  // Calculate memory
  memorySize+=sizeof(*this);

  // Write the size of the object for detecting changes later
  Int size=sizeof(*this);
  Storage::storeInt(ofs,size);

  // Store all relevant fields
  Storage::storeInt(ofs,mapX[0]);
  Storage::storeInt(ofs,mapY[0]);
  Storage::storeDouble(ofs,lngX[0]);
  Storage::storeDouble(ofs,lngX[1]);
  Storage::storeDouble(ofs,latY[0]);
  Storage::storeDouble(ofs,latY[1]);
  Storage::storeDouble(ofs,latNorthMax);
  Storage::storeDouble(ofs,latNorthMin);
  Storage::storeDouble(ofs,latSouthMax);
  Storage::storeDouble(ofs,latSouthMin);
  Storage::storeDouble(ofs,lngWestMax);
  Storage::storeDouble(ofs,lngWestMin);
  Storage::storeDouble(ofs,lngEastMax);
  Storage::storeDouble(ofs,lngEastMin);
  Storage::storeDouble(ofs,northAngle);
  Storage::storeDouble(ofs,lngScale);
  Storage::storeDouble(ofs,latScale);

}

// Reads the contents of the object from a binary file
MapTile *MapTile::retrieve(char *&cacheData, Int &cacheSize, char *&objectData, Int &objectSize, MapContainer *parent) {

  //PROFILE_START;

  // Check if the class has changed
  Int size=sizeof(MapTile);
#ifdef TARGET_LINUX
  if (size!=728) {
    FATAL("unknown size of object (%d), please adapt class storage",size);
    return NULL;
  }
#endif

  // Read the size of the object and check with current size
  size=0;
  Storage::retrieveInt(cacheData,cacheSize,size);
  if (size!=sizeof(MapTile)) {
    DEBUG("stored size of object does not match implemented object size, aborting retrieve",NULL);
    return NULL;
  }
  //PROFILE_ADD("sanity check");

  // Read the position of the tile
  Int x,y;
  Storage::retrieveInt(cacheData,cacheSize,x);
  Storage::retrieveInt(cacheData,cacheSize,y);
  //PROFILE_ADD("field read");

  // Create a new object
  MapTile *mapTile=NULL;
  objectSize-=sizeof(MapTile);
  if (objectSize<0) {
    DEBUG("can not create map tile object",NULL);
    return NULL;
  }
  mapTile=new(objectData) MapTile(x,y,parent,true,true);
  objectData+=sizeof(MapTile);
  //PROFILE_ADD("object creation");

  // Retrieve the remaining fields
  Storage::retrieveDouble(cacheData,cacheSize,mapTile->lngX[0]);
  Storage::retrieveDouble(cacheData,cacheSize,mapTile->lngX[1]);
  Storage::retrieveDouble(cacheData,cacheSize,mapTile->latY[0]);
  Storage::retrieveDouble(cacheData,cacheSize,mapTile->latY[1]);
  Storage::retrieveDouble(cacheData,cacheSize,mapTile->latNorthMax);
  Storage::retrieveDouble(cacheData,cacheSize,mapTile->latNorthMin);
  Storage::retrieveDouble(cacheData,cacheSize,mapTile->latSouthMax);
  Storage::retrieveDouble(cacheData,cacheSize,mapTile->latSouthMin);
  Storage::retrieveDouble(cacheData,cacheSize,mapTile->lngWestMax);
  Storage::retrieveDouble(cacheData,cacheSize,mapTile->lngWestMin);
  Storage::retrieveDouble(cacheData,cacheSize,mapTile->lngEastMax);
  Storage::retrieveDouble(cacheData,cacheSize,mapTile->lngEastMin);
  Storage::retrieveDouble(cacheData,cacheSize,mapTile->northAngle);
  Storage::retrieveDouble(cacheData,cacheSize,mapTile->lngScale);
  Storage::retrieveDouble(cacheData,cacheSize,mapTile->latScale);

  // Return result
  return mapTile;
}

// Compares the latitude and longitude positions with the one of the given tile
bool MapTile::isNeighbor(MapTile *t, Int ownEdgeX, Int ownEdgeY, Int foreignEdgeX, Int foreignEdgeY) const {
  double diffLat=fabs(getLatY(ownEdgeY)-t->getLatY(foreignEdgeY));
  double diffX=diffLat*latScale;
  double diffLng=fabs(getLngX(ownEdgeX)-t->getLngX(foreignEdgeX));
  double diffY=diffLng*latScale;
  DEBUG("diffX=%e diffY=%e",diffX,diffY);
  if ((diffX<core->getMapSource()->getNeighborPixelTolerance())&&(diffY<core->getMapSource()->getNeighborPixelTolerance()))
    return true;
  else
    return false;
}

// Transfers the current visual position to the rectangle
void MapTile::activateVisPos()
{
  visualization.setX(visX);
  visualization.setY(visY);
  visualization.setZ(visZ);
}

// Decides if the tile is drawn on screen
void MapTile::setIsHidden(bool isHidden) {
  this->isHidden=isHidden;
  GraphicColor c=visualization.getColor();
  if (isHidden) {
    c.setAlpha(0);
  } else {
    c.setAlpha(255);
  }
  visualization.setColor(c);
}

// Destructs the objects correctly (i.e., if memory has not been allocated by new)
void MapTile::destruct(MapTile *object) {
  if (object->doNotDelete) {
    object->~MapTile();
  } else {
    delete object;
  }
}

// Called when the tile is removed from the screen
void MapTile::graphicInvalidated() {

  // Reset visualization key
  setVisualizationKey(0);

  // Invalidate all graphic
  visualization.lockAccess();
  visualization.invalidate();
  visualization.unlockAccess();

}

}