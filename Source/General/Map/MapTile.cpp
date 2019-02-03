//============================================================================
// Name        : MapTile.cpp
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

namespace GEODISCOVERER {

MapTile::MapTile(Int mapX, Int mapY, MapContainer *parent, bool doNotInit, bool doNotDelete) : rectangle(core->getDefaultScreen()), visualization(core->getDefaultScreen()) {

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
  endTexture=Screen::getTextureNotDefined();
  visualization.setColor(GraphicColor(255,255,255,0));

  // Init the remaining object
  if (!doNotInit)
    init();
}

MapTile::~MapTile() {
  resetVisualization();
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
  if (visualization.getPrimitiveMap()->size()!=1) {
    FATAL("expected only the rectangle in the visualization object",NULL);
  }
  core->getDefaultGraphicEngine()->unlockDrawing();
  for (MapTileNavigationPathMap::iterator i=crossingPathSegmentsMap.begin();i!=crossingPathSegmentsMap.end();i++) {
    std::list<NavigationPathSegment*> *pathSegments = i->second;
    for (std::list<NavigationPathSegment*>::iterator j=pathSegments->begin();j!=pathSegments->end();j++) {
      delete *j;
    }
    delete pathSegments;
  }
  crossingPathSegmentsMap.clear();
  resetVisualization();
}

// Removes all overlay graphics from the visualization
void MapTile::resetVisualization() {
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
  for (std::list<GraphicPrimitiveKey>::iterator i=retrievedPrimitives.begin();i!=retrievedPrimitives.end();i++) {
    visualization.removePrimitive(*i,true);
  }
  core->getDefaultGraphicEngine()->unlockDrawing();
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
  if (size!=1408) {
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
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
  visualization.recreateGraphic();
  core->getDefaultGraphicEngine()->unlockDrawing();

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
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
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
        rectangle.setFadeAnimationSequence(std::list<GraphicFadeAnimationParameter>());
        rectangle.setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),startColor,endColor,false,core->getDefaultGraphicEngine()->getFadeDuration());
        rectangle.setTextureAnimationSequence(std::list<GraphicTextureAnimationParameter>());
        rectangle.setTextureAnimation(core->getClock()->getMicrosecondsSinceStart(),endTexture,endTexture,false,0);
      } else {
        std::list<GraphicFadeAnimationParameter> fadeAnimationSequence;
        GraphicFadeAnimationParameter fadeAnimationParameter;
        std::list<GraphicTextureAnimationParameter> textureAnimationSequence;
        GraphicTextureAnimationParameter textureAnimationParameter;
        fadeAnimationParameter.setStartTime(core->getClock()->getMicrosecondsSinceStart());
        textureAnimationParameter.setStartTime(fadeAnimationParameter.getStartTime());
        TimestampInMicroseconds duration=rectangle.getColor().getAlpha()*core->getDefaultGraphicEngine()->getFadeDuration()/255;
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
        fadeAnimationParameter.setDuration(core->getDefaultGraphicEngine()->getFadeDuration());
        fadeAnimationSequence.push_back(fadeAnimationParameter);
        rectangle.setFadeAnimationSequence(fadeAnimationSequence);
        rectangle.setTextureAnimationSequence(textureAnimationSequence);
      }
    } else {
      rectangle.setTexture(endTexture);
      rectangle.setFadeAnimationSequence(std::list<GraphicFadeAnimationParameter>());
      rectangle.setFadeAnimation(core->getClock()->getMicrosecondsSinceStart(),endColor,endColor,false,0);
      rectangle.setTextureAnimationSequence(std::list<GraphicTextureAnimationParameter>());
      rectangle.setTextureAnimation(core->getClock()->getMicrosecondsSinceStart(),endTexture,endTexture,false,0);
    }
  }
  core->getDefaultGraphicEngine()->unlockDrawing();
}

// Updates the cache status
void MapTile::setIsCached(bool isCached, GraphicTextureInfo texture, bool fadeOutAnimation)
{
  this->isCached = isCached;
  if (!isCached) {
    if (this->getParentMapContainer()->getDownloadComplete()) {
      if (this->getParentMapContainer()->getDownloadErrorOccured())
        endTexture=core->getDefaultGraphicEngine()->getDownloadErrorOccuredTileImage()->getTexture();
      else
        endTexture=core->getDefaultGraphicEngine()->getNotCachedTileImage()->getTexture();
    } else {
      endTexture=core->getDefaultGraphicEngine()->getNotDownloadedTileImage()->getTexture();
    }
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

// Stores an animator
void MapTile::storeAnimator(std::ofstream *ofs, GraphicPrimitive *animator) {
  Storage::storeString(ofs,animator->getName().front());
  Storage::storeGraphicColor(ofs,animator->getFadeStartColor());
  Storage::storeGraphicColor(ofs,animator->getFadeEndColor());
  Storage::storeInt(ofs,(Int)animator->getFadeDuration());
  Storage::storeBool(ofs,animator->getFadeInfinite());
}

// Retrieves an animator
GraphicPrimitive *MapTile::retrieveAnimator(char *&data, Int &size) {

  // Read all fields
  char *name;
  Storage::retrieveString(data,size,&name);
  //DEBUG("name=%s",name);
  GraphicColor startColor;
  Storage::retrieveGraphicColor(data,size,startColor);
  //DEBUG("r=%d g=%d b=%d a=%d",startColor.getRed(),startColor.getGreen(),startColor.getBlue(),startColor.getAlpha());
  GraphicColor endColor;
  Storage::retrieveGraphicColor(data,size,endColor);
  //DEBUG("r=%d g=%d b=%d a=%d",endColor.getRed(),endColor.getGreen(),endColor.getBlue(),endColor.getAlpha());
  Int duration;
  Storage::retrieveInt(data,size,duration);
  //DEBUG("duration=%d",duration);
  bool infinite;
  Storage::retrieveBool(data,size,infinite);
  //DEBUG("infinite=%d",infinite);

  // Check if the animator is already known
  GraphicPrimitive *animator;
  animator=core->getMapSource()->findPathAnimator(name);
  if (!animator) {
    //DEBUG("creating new animator for %s",name);
    GraphicPrimitiveKey animatorKey;
    GraphicObject *pathAnimators;
    if (!(animator=new GraphicPrimitive(core->getDefaultScreen()))) {
      FATAL("can not create graphic primitive object",NULL);
      return NULL;
    }
    animator->setFadeAnimation(0,startColor,endColor,infinite,duration);
    std::list<std::string> names;
    names.push_back(name);
    animator->setName(names);
    pathAnimators=core->getDefaultGraphicEngine()->lockPathAnimators(__FILE__, __LINE__);
    animatorKey=pathAnimators->addPrimitive(animator);
    core->getDefaultGraphicEngine()->unlockPathAnimators();
    core->getMapSource()->addPathAnimator(animatorKey);
  } else {
    //DEBUG("reusing already known animator for %s",name);
  }
  return animator;
}

// Stores the overlayed graphics into a file (excluding the tile itself)
void MapTile::storeOverlayGraphics(std::ofstream *ofs) {

  Int size;
  std::list<GraphicPointBuffer*> *lineSegments;
  std::list<GraphicRectangleListSegment*> *rectangleListSegments;

  GraphicPrimitive *animator;
  GraphicLine *line;
  GraphicRectangleList *rectangleList;

  // Iterate through the graphics object
  std::list<GraphicPrimitive*> *drawList = visualization.getDrawList();
  for(std::list<GraphicPrimitive*>::iterator i=drawList->begin();i!=drawList->end();i++) {

    // Which type?
    switch((*i)->getType()) {

    // Line object from a navigation path
    case GraphicTypeLine:

      // Store the important fields
      line=(GraphicLine*)*i;
      Storage::storeByte(ofs,(*i)->getType());
      animator=line->getAnimator();
      storeAnimator(ofs,animator);
      Storage::storeShort(ofs,line->getWidth());
      Storage::storeInt(ofs,line->getZ());
      Storage::storeInt(ofs,line->getCutWidth());
      Storage::storeInt(ofs,line->getCutHeight());
      lineSegments=line->getSegments();
      size=0;
      for(std::list<GraphicPointBuffer*>::iterator j=lineSegments->begin();j!=lineSegments->end();j++) {
        size+=(*j)->getSize();
      }
      Storage::storeInt(ofs,size);
      for(std::list<GraphicPointBuffer*>::iterator j=lineSegments->begin();j!=lineSegments->end();j++) {
        GraphicPointBuffer *pointBuffer=*j;
        for(Int k=0;k<pointBuffer->getSize();k++) {
          Short x,y;
          pointBuffer->getPoint(k,x,y);
          Storage::storeShort(ofs,x);
          Storage::storeShort(ofs,y);
        }
      }
      break;

    // Arrow object from a navigation path
    case GraphicTypeRectangleList:

      // Store the important fields
      rectangleList=(GraphicRectangleList*)*i;
      Storage::storeByte(ofs,(*i)->getType());
      animator=rectangleList->getAnimator();
      storeAnimator(ofs,animator);
      Storage::storeInt(ofs,rectangleList->getZ());
      Storage::storeInt(ofs,rectangleList->getCutWidth());
      Storage::storeInt(ofs,rectangleList->getCutHeight());
      Storage::storeAlignment(ofs,sizeof(double));
      Storage::storeDouble(ofs,rectangleList->getRadius());
      Storage::storeDouble(ofs,rectangleList->getDistanceToCenter());
      Storage::storeDouble(ofs,rectangleList->getAngleToCenter());
      rectangleListSegments=rectangleList->getSegments();
      size=0;
      for(std::list<GraphicRectangleListSegment*>::iterator j=rectangleListSegments->begin();j!=rectangleListSegments->end();j++) {
        size+=(*j)->getRectangleCount();
      }
      Storage::storeInt(ofs,size);
      for(std::list<GraphicRectangleListSegment*>::iterator j=rectangleListSegments->begin();j!=rectangleListSegments->end();j++) {
        GraphicRectangleListSegment *rectangleListSegment=*j;
        for(Int k=0;k<rectangleListSegment->getRectangleCount();k++) {
          Short x[4],y[4];
          rectangleListSegment->getRectangle(k,&x[0],&y[0]);
          for(Int l=0;l<4;l++) {
            Storage::storeShort(ofs,x[l]);
            Storage::storeShort(ofs,y[l]);
          }
        }
      }
      break;

    default:
      //DEBUG("graphic primitive type %d not yet implemented",(*i)->getType());
      break;
    }
  }

  // Mark the end
  Storage::storeByte(ofs,127);

  // That's it
  return;
}

// Recreates the visualization from a binary file
void MapTile::retrieveOverlayGraphics(char *&data, Int &size) {

  GraphicPrimitiveKey lineKey;
  GraphicPrimitiveKey rectangleListKey;
  GraphicPrimitive *animator;
  Short width;
  Int temp;
  double radius,distance,angle;

  // Remove all overlayed graphics
  resetVisualization();

  // Get current time
  TimestampInMicroseconds t=core->getClock()->getMicrosecondsSinceStart();

  // Repeat until all data is consumed
  bool complete=false;
  while (!complete) {

    // Get the type of graphic to create
    Byte type;
    Storage::retrieveByte(data,size,type);
    //DEBUG("type=%d",type);
    switch(type) {

    // Line object from navigation path?
    case GraphicTypeLine:

      // Create the animator
      animator=retrieveAnimator(data,size);

      // Create the line
      Storage::retrieveShort(data,size,width);
      GraphicLine *line;
      if (!(line=new GraphicLine(core->getDefaultScreen(),0,width))) {
        FATAL("can not create graphic primitive object",NULL);
      }
      line->setAnimator(animator);
      Storage::retrieveInt(data,size,temp);
      line->setZ(temp);
      line->setCutEnabled(true);
      Storage::retrieveInt(data,size,temp);
      line->setCutWidth(t);
      Storage::retrieveInt(data,size,temp);
      line->setCutHeight(t);
      Int numberOfPoints;
      Storage::retrieveInt(data,size,numberOfPoints);
      for (Int i=0;i<numberOfPoints;i++) {
        Short x,y;
        Storage::retrieveShort(data,size,x);
        Storage::retrieveShort(data,size,y);
        line->addPoint(x,y);
      }
      line->optimize();

      // Add it to the visualization
      core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
      lineKey=visualization.addPrimitive(line);
      core->getDefaultGraphicEngine()->unlockDrawing();
      retrievedPrimitives.push_back(lineKey);
      break;

    // Arrow object from navigation path?
    case GraphicTypeRectangleList:

      // Create the animator
      animator=retrieveAnimator(data,size);

      // Create the rectangle list
      GraphicRectangleList *rectangleList;
      if (!(rectangleList=new GraphicRectangleList(core->getDefaultScreen(),0))) {
        FATAL("can not create graphic primitive object",NULL);
      }
      rectangleList->setAnimator(animator);
      Storage::retrieveInt(data,size,temp);
      rectangleList->setZ(temp);
      rectangleList->setTexture(core->getDefaultGraphicEngine()->getPathDirectionIcon()->getTexture());
      rectangleList->setDestroyTexture(false);
      rectangleList->setCutEnabled(true);
      Storage::retrieveInt(data,size,temp);
      rectangleList->setCutWidth(temp);
      Storage::retrieveInt(data,size,temp);
      rectangleList->setCutHeight(temp);
      Storage::retrieveAlignment(data,size,sizeof(double));
      Storage::retrieveDouble(data,size,radius);
      Storage::retrieveDouble(data,size,distance);
      Storage::retrieveDouble(data,size,angle);
      rectangleList->setParameter(radius,distance,angle);
      Int numberOfRectangles;
      Storage::retrieveInt(data,size,numberOfRectangles);
      for (Int i=0;i<numberOfRectangles;i++) {
        Short x[4],y[4];
        for(Int k=0;k<4;k++) {
          Storage::retrieveShort(data,size,x[k]);
          Storage::retrieveShort(data,size,y[k]);
        }
        rectangleList->addRectangle(x,y);
      }
      rectangleList->optimize();

      // Add it to the visualization
      core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
      rectangleListKey=visualization.addPrimitive(rectangleList);
      core->getDefaultGraphicEngine()->unlockDrawing();
      retrievedPrimitives.push_back(rectangleListKey);
      break;

    // End?
    case 127:
      complete=true;
      break;
    }
  }
}

// Indicates that textures and buffers shall be created
void MapTile::createGraphic() {
  for (std::list<GraphicPrimitiveKey>::iterator i=retrievedPrimitives.begin();i!=retrievedPrimitives.end();i++) {
    GraphicPrimitive *p=visualization.getPrimitive(*i);
    switch (p->getType()) {
    case GraphicTypeLine:
      break;
    case GraphicTypeRectangleList:
      ((GraphicRectangleList*)p)->setTexture(core->getDefaultGraphicEngine()->getPathDirectionIcon()->getTexture());
      break;
    default:
      FATAL("unsupported graphic primtive found",NULL);
    }
  }
}

// Indicates that textures and buffers have been cleared
void MapTile::destroyGraphic() {
  for (std::list<GraphicPrimitiveKey>::iterator i=retrievedPrimitives.begin();i!=retrievedPrimitives.end();i++) {
    GraphicPrimitive *p=visualization.getPrimitive(*i);
    switch (p->getType()) {
    case GraphicTypeLine:
      ((GraphicLine*)p)->invalidate();
      break;
    case GraphicTypeRectangleList:
      ((GraphicRectangleList*)p)->invalidate();
      break;
    default:
      FATAL("unsupported graphic primtive found",NULL);
    }
  }
}

}
