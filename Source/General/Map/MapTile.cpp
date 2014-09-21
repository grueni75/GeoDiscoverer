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
  this->isHidden=true;
  //DEBUG("width=%d height=%d",width,height);
  //PROFILE_ADD("basic variable copy")
  this->rectangle.setWidth(width);
  this->rectangle.setHeight(height);
  this->rectangle.setDestroyTexture(false);
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
  endTexture=core->getScreen()->getTextureNotDefined();
  visualization.setColor(GraphicColor(255,255,255,0));

  // Init the remaining object
  if (!doNotInit)
    init();
}

MapTile::~MapTile() {
  core->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
  if (visualization.getPrimitiveMap()->size()!=1) {
    FATAL("expected only the rectangle in the visualization object",NULL);
  }
  core->getGraphicEngine()->unlockDrawing();
  for (MapTileNavigationPathMap::iterator i=crossingPathSegmentsMap.begin();i!=crossingPathSegmentsMap.end();i++) {
    std::list<NavigationPathSegment*> *pathSegments = i->second;
    for (std::list<NavigationPathSegment*>::iterator j=pathSegments->begin();j!=pathSegments->end();j++) {
      delete *j;
    }
    delete pathSegments;
  }
  crossingPathSegmentsMap.clear();
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
void MapTile::store(std::ofstream *ofs) {

  // Write the size of the object for detecting changes later
  Int size=sizeof(*this);
  Storage::storeInt(ofs,size);

  // Store all relevant fields
  MapTile *leftChild=this->leftChild;
  MapTile *rightChild=this->rightChild;
  this->leftChild=NULL;
  this->rightChild=NULL;
  Storage::storeMem(ofs,(char*)this,sizeof(MapTile),true);
  this->leftChild=leftChild;
  this->rightChild=rightChild;
}

// Reads the contents of the object from a binary file
MapTile *MapTile::retrieve(char *&cacheData, Int &cacheSize, MapContainer *parent) {

  //PROFILE_START;

  // Check if the class has changed
  Int size=sizeof(MapTile);
#ifdef TARGET_LINUX
  if (size!=1288) {
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

  // Create a new object
  Storage::skipPadding(cacheData,cacheSize);
  cacheSize-=sizeof(MapTile);
  if (cacheSize<0) {
    DEBUG("can not create map tile object",NULL);
    return NULL;
  }
  MapTile *mapTile=(MapTile*)cacheData;
  mapTile=new(cacheData) MapTile(mapTile->mapX[0],mapTile->mapY[0],parent,true,true);
  cacheData+=sizeof(MapTile);
  //PROFILE_ADD("object creation");

  // Return result
  return mapTile;
}

// Compares the latitude and longitude positions with the one of the given tile
bool MapTile::isNeighbor(MapTile *t, Int ownEdgeX, Int ownEdgeY, Int foreignEdgeX, Int foreignEdgeY) const {
  double diffLat=fabs(getLatY(ownEdgeY)-t->getLatY(foreignEdgeY));
  double diffX=diffLat*latScale;
  double diffLng=fabs(getLngX(ownEdgeX)-t->getLngX(foreignEdgeX));
  double diffY=diffLng*latScale;
  //DEBUG("diffX=%e diffY=%e",diffX,diffY);
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

// Destructs the objects correctly (i.e., if memory has not been allocated by new)
void MapTile::destruct(MapTile *object) {
  if (object->doNotDelete) {
    object->~MapTile();
  } else {
    delete object;
  }
}

// Called when the tile is removed from the screen
void MapTile::removeGraphic() {

  // Reset visualization key
  setVisualizationKey(0);

  // Invalidate all graphic
  core->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
  visualization.recreateGraphic();
  core->getGraphicEngine()->unlockDrawing();

}

// Checks if the area is a neighbor of the tile and computes the center position of the neighbor
bool MapTile::getNeighborPos(MapArea area, MapPosition &neighborPos) {

  MapPosition pos;

  // Prepare variables
  double lngLen = lngEastMax-lngWestMin;
  double latLen = latNorthMax-latSouthMin;
  double latCenter = latSouthMin+latLen/2;
  double lngCenter = lngWestMin+lngLen/2;

  // North neighbor?
  pos.setLng(lngCenter);
  pos.setLat(latCenter+latLen);
  if (area.containsGeographicCoordinate(pos)) {
    neighborPos=pos;
    return true;
  }

  // North east neighbor?
  pos.setLng(lngCenter+lngLen);
  pos.setLat(latCenter+latLen);
  if (area.containsGeographicCoordinate(pos)) {
    neighborPos=pos;
    return true;
  }

  // East neighbor?
  pos.setLng(lngCenter+lngLen);
  pos.setLat(latCenter);
  if (area.containsGeographicCoordinate(pos)) {
    neighborPos=pos;
    return true;
  }

  // South east neighbor?
  pos.setLng(lngCenter+lngLen);
  pos.setLat(latCenter-latLen);
  if (area.containsGeographicCoordinate(pos)) {
    neighborPos=pos;
    return true;
  }

  // South neighbor?
  pos.setLng(lngCenter);
  pos.setLat(latCenter-latLen);
  if (area.containsGeographicCoordinate(pos)) {
    neighborPos=pos;
    return true;
  }

  // South west neighbor?
  pos.setLng(lngCenter-lngLen);
  pos.setLat(latCenter-latLen);
  if (area.containsGeographicCoordinate(pos)) {
    neighborPos=pos;
    return true;
  }

  // West neighbor?
  pos.setLng(lngCenter-lngLen);
  pos.setLat(latCenter);
  if (area.containsGeographicCoordinate(pos)) {
    neighborPos=pos;
    return true;
  }

  // North west neighbor?
  pos.setLng(lngCenter-lngLen);
  pos.setLat(latCenter+latLen);
  if (area.containsGeographicCoordinate(pos)) {
    neighborPos=pos;
    return true;
  }

  // No neighbor found
  return false;

}

// Decides if the tile is drawn on screen
void MapTile::setIsHidden(bool isHidden, const char *file, int line, bool fadeOutAnimation) {
  this->isHidden=isHidden;
  core->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
  GraphicColor startColor=visualization.getColor();
  startColor.setAlpha(0);
  GraphicColor endColor=startColor;
  endColor.setAlpha(255);
  if (isHidden) {
    visualization.setColor(startColor);
    rectangle.setTexture(endTexture);
  } else {
    visualization.setColor(endColor);
    if ((isDrawn())&&(fadeOutAnimation)) {
      if (rectangle.getTexture()==endTexture) {
        rectangle.setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),startColor,endColor,false,core->getGraphicEngine()->getFadeDuration());
        rectangle.setFadeAnimationSequence(std::list<GraphicFadeAnimationParameter>());
        rectangle.setTextureAnimationSequence(std::list<GraphicTextureAnimationParameter>());
      } else {
        std::list<GraphicFadeAnimationParameter> fadeAnimationSequence;
        GraphicFadeAnimationParameter fadeAnimationParameter;
        std::list<GraphicTextureAnimationParameter> textureAnimationSequence;
        GraphicTextureAnimationParameter textureAnimationParameter;
        fadeAnimationParameter.setStartTime(core->getClock()->getMicrosecondsSinceStart());
        textureAnimationParameter.setStartTime(fadeAnimationParameter.getStartTime());
        TimestampInMicroseconds duration=rectangle.getColor().getAlpha()*core->getGraphicEngine()->getFadeDuration()/255;
        fadeAnimationParameter.setDuration(duration);
        textureAnimationParameter.setDuration(duration);
        fadeAnimationParameter.setStartColor(rectangle.getColor());
        textureAnimationParameter.setStartTexture(rectangle.getTexture());
        fadeAnimationParameter.setEndColor(startColor);
        textureAnimationParameter.setEndTexture(endTexture);
        fadeAnimationSequence.push_back(fadeAnimationParameter);
        textureAnimationSequence.push_back(textureAnimationParameter);
        fadeAnimationParameter.setStartTime(fadeAnimationParameter.getStartTime()+duration);
        fadeAnimationParameter.setStartColor(startColor);
        fadeAnimationParameter.setEndColor(endColor);
        fadeAnimationParameter.setDuration(core->getGraphicEngine()->getFadeDuration());
        fadeAnimationSequence.push_back(fadeAnimationParameter);
        rectangle.setFadeAnimationSequence(fadeAnimationSequence);
        rectangle.setTextureAnimationSequence(textureAnimationSequence);
      }
    } else {
      rectangle.setTexture(endTexture);
      rectangle.setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),endColor,endColor,false,0);
      rectangle.setFadeAnimationSequence(std::list<GraphicFadeAnimationParameter>());
      rectangle.setTextureAnimationSequence(std::list<GraphicTextureAnimationParameter>());
    }
  }
  core->getGraphicEngine()->unlockDrawing();
}

// Updates the cache status
void MapTile::setIsCached(bool isCached, GraphicTextureInfo texture, bool fadeOutAnimation)
{
  this->isCached = isCached;
  if (!isCached) {
    if (this->getParentMapContainer()->getDownloadComplete())
      endTexture=core->getGraphicEngine()->getNotCachedTileImage()->getTexture();
    else
      endTexture=core->getGraphicEngine()->getNotDownloadedTileImage()->getTexture();
  } else {
    endTexture=texture;
  }
  setIsHidden(isHidden, __FILE__, __LINE__, fadeOutAnimation);
}

// Adds a new segment that crosses this map tile
void MapTile::addCrossingNavigationPathSegment(NavigationPath *path, NavigationPathSegment *segment) {
  MapTileNavigationPathMap::iterator i;
  i=crossingPathSegmentsMap.find(path);
  std::list<NavigationPathSegment*> *pathSegments=NULL;
  if (i==crossingPathSegmentsMap.end()) {
    if (!(pathSegments=new std::list<NavigationPathSegment*>())) {
      FATAL("can not create list object of navigation path segments",NULL);
      return;
    }
    crossingPathSegmentsMap.insert(MapTileNavigationPathPair(path,pathSegments));
  } else {
    pathSegments=i->second;
  }
  pathSegments->push_back(segment);
}

// Returns the segments of the given path that cross this tile
std::list<NavigationPathSegment*>* MapTile::findCrossingNavigationPathSegments(NavigationPath *path) {
  MapTileNavigationPathMap::iterator i=crossingPathSegmentsMap.find(path);
  if (i!=crossingPathSegmentsMap.end()) {
    return i->second;
  } else {
    return NULL;
  }
}

// Returns a list of path segments that cross this tile
std::list<NavigationPathSegment*> MapTile::getCrossingNavigationPathSegments() {
  std::list<NavigationPathSegment*> allPathSegments;
  for (MapTileNavigationPathMap::iterator i=crossingPathSegmentsMap.begin();i!=crossingPathSegmentsMap.end();i++) {
    std::list<NavigationPathSegment*> *pathSegments = i->second;
    for (std::list<NavigationPathSegment*>::iterator j=pathSegments->begin();j!=pathSegments->end();j++) {
      allPathSegments.push_back(*j);
    }
  }
  return allPathSegments;
}

// Removes all segments that belong to the given path
void MapTile::removeCrossingNavigationPathSegments(NavigationPath *path) {
  MapTileNavigationPathMap::iterator i=crossingPathSegmentsMap.find(path);
  if (i!=crossingPathSegmentsMap.end()) {
    std::list<NavigationPathSegment*> *pathSegments = i->second;
    for (std::list<NavigationPathSegment*>::iterator j=pathSegments->begin();j!=pathSegments->end();j++) {
      delete *j;
    }
    delete pathSegments;
    crossingPathSegmentsMap.erase(i);
  }
}

}
