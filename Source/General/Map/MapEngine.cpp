//============================================================================
// Name        : MapEngine.cpp
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

// Constructor
MapEngine::MapEngine() {
  initDistance=core->getConfigStore()->getIntValue("Map","initDistance");
  maxTiles=core->getConfigStore()->getIntValue("Map","maxTiles");
  returnToLocationTimeout=core->getConfigStore()->getIntValue("Map","returnToLocationTimeout");
  returnToLocationOneTime=false;
  returnToLocation=false;
  setReturnToLocation(core->getConfigStore()->getIntValue("Map","returnToLocation"));
  setZoomLevelLock(core->getConfigStore()->getIntValue("Map","zoomLevelLock"));
  map=NULL;
  updateInProgress=false;
  abortUpdate=false;
  forceMapUpdate=false;
  forceMapRecreation=false;
  forceCacheUpdate=false;
  locationPosMutex=core->getThread()->createMutex();
  mapPosMutex=core->getThread()->createMutex();
  compassBearingMutex=core->getThread()->createMutex();
  displayAreaMutex=core->getThread()->createMutex();
  forceCacheUpdateMutex=core->getThread()->createMutex();
  forceMapUpdateMutex=core->getThread()->createMutex();
  prevCompassBearing=-std::numeric_limits<double>::max();
  compassBearing=prevCompassBearing;
  isInitialized=false;
}

// Destructor
MapEngine::~MapEngine() {
  deinitMap();
  core->getThread()->destroyMutex(locationPosMutex);
  core->getThread()->destroyMutex(mapPosMutex);
  core->getThread()->destroyMutex(compassBearingMutex);
  core->getThread()->destroyMutex(displayAreaMutex);
  core->getThread()->destroyMutex(forceCacheUpdateMutex);
  core->getThread()->destroyMutex(forceMapUpdateMutex);
}

// Does all action to remove a tile from the map
void MapEngine::deinitTile(MapTile *t)
{
  map->lockAccess();
  map->removePrimitive(t->getVisualizationKey()); // Remove it from the graphic
  t->removeGraphic();
  if (t->getIsDummy())   // Delete dummy tiles
    delete t;
  map->unlockAccess();
}

// Clear the current map
void MapEngine::deinitMap()
{
  DEBUG("before deinit map",NULL);

  // Deinit all tiles
  //DEBUG("deiniting tiles",NULL);
  for (std::list<MapTile*>::const_iterator i=tiles.begin();i!=tiles.end();i++) {
    MapTile *t=*i;
    t->setIsProcessed(false);
    deinitTile(t);
  }
  tiles.clear();

  // Delete all left-over primitives that were used for debugging
  //DEBUG("deleting debug primitives",NULL);
  if (map) {
    map->lockAccess();
    GraphicPrimitiveMap::iterator i;
    GraphicPrimitiveMap *primitiveMap = map->getPrimitiveMap();
    std::list<GraphicPrimitive*> primitives;
    std::list<GraphicPrimitiveKey> keys;
    for(i = primitiveMap->begin(); i!=primitiveMap->end(); i++) {
      GraphicPrimitiveKey key;
      GraphicPrimitive *primitive;
      key=i->first;
      primitive=i->second;
      primitives.push_back(primitive);
      keys.push_back(key);
    }
    primitiveMap->clear();
    for(std::list<GraphicPrimitiveKey>::const_iterator i=keys.begin(); i != keys.end(); i++) {
      map->removePrimitive(*i);
    }
    for(std::list<GraphicPrimitive*>::const_iterator i=primitives.begin(); i != primitives.end(); i++) {
      delete *i;
    }
    map->unlockAccess();
  }

  // Free graphic objects
  //DEBUG("deleting map",NULL);
  if (map) {
    map->lockAccess();
    core->getGraphicEngine()->setMap(NULL);
    map->unlockAccess();
    delete map;
    map=NULL;
  }

  // Object is not initialized anymore
  isInitialized=false;

  DEBUG("after deinit map",NULL);
}

// Initializes the map
void MapEngine::initMap()
{
  // Clear the current map
  deinitMap();

  // Get the current position from the graphic engine
  GraphicPosition *graphicEngineVisPos=core->getGraphicEngine()->lockPos();
  visPos=*graphicEngineVisPos;
  core->getGraphicEngine()->unlockPos();

  // Set the current position
  std::string lastMapFolder=core->getConfigStore()->getStringValue("Map/LastPosition","folder");
  if (lastMapFolder!="*unknown*") {
    lockMapPos();
    mapPos.setLng(core->getConfigStore()->getDoubleValue("Map/LastPosition","lng"));
    mapPos.setLat(core->getConfigStore()->getDoubleValue("Map/LastPosition","lat"));
    mapPos.setLatScale(core->getConfigStore()->getDoubleValue("Map/LastPosition","latScale"));
    mapPos.setLngScale(core->getConfigStore()->getDoubleValue("Map/LastPosition","lngScale"));
    unlockMapPos();
    core->getMapSource()->lockAccess();
    MapTile *tile=core->getMapSource()->findMapTileByGeographicCoordinate(mapPos,0,false,NULL);
    core->getMapSource()->unlockAccess();
    if (tile==NULL) {
      DEBUG("could not find tile, using center position of map",NULL);
      lockMapPos();
      mapPos=core->getMapSource()->getCenterPosition();
      unlockMapPos();
    }
  } else {
    lockMapPos();
    mapPos=*(core->getMapSource()->getCenterPosition());
    unlockMapPos();
  }
  if (lastMapFolder==core->getMapSource()->getFolder()) {
    visPos.setZoom(core->getConfigStore()->getDoubleValue("Map/LastPosition","zoom"));
    visPos.setAngle(core->getConfigStore()->getDoubleValue("Map/LastPosition","angle"));
    lockDisplayArea();
    displayArea.setZoomLevel(core->getConfigStore()->getDoubleValue("Map/LastPosition","zoomLevel"));
    unlockDisplayArea();
  }

  // Store the new values
  backup();

  // Update the visual position in the graphic engine
  graphicEngineVisPos=core->getGraphicEngine()->lockPos();
  *graphicEngineVisPos=visPos;
  core->getGraphicEngine()->unlockPos();

  // Create new graphic objects
  map=new GraphicObject();
  if (!map) {
    FATAL("can not create graphic object for the map",NULL);
    return;
  }

  // Inform the graphic engine about the new objects
  map->lockAccess();
  core->getGraphicEngine()->setMap(map);
  map->unlockAccess();

  // Force redraw
  forceMapRecreation=true;

  // Object is initialized
  isInitialized=true;
}

// Remove all debugging primitives
void MapEngine::removeDebugPrimitives() {
  map->lockAccess();
  GraphicPrimitiveMap::iterator i;
  GraphicPrimitiveMap *primitiveMap = map->getPrimitiveMap();
  std::list<GraphicPrimitive*> primitives;
  std::list<GraphicPrimitiveKey> keys;
  for(i = primitiveMap->begin(); i!=primitiveMap->end(); i++) {
    GraphicPrimitiveKey key;
    GraphicPrimitive *primitive;
    key=i->first;
    primitive=i->second;
    if ((primitive->getName().size()==1)&&(primitive->getName().front()=="")) {
      primitives.push_back(primitive);
      keys.push_back(key);
    }
  }
  for(std::list<GraphicPrimitiveKey>::const_iterator i=keys.begin(); i != keys.end(); i++) {
    map->removePrimitive(*i);
  }
  for(std::list<GraphicPrimitive*>::const_iterator i=primitives.begin(); i != primitives.end(); i++) {
    delete *i;
  }
  map->unlockAccess();
}

// Fills the given area with tiles
void MapEngine::fillGeographicAreaWithTiles(MapArea area, MapTile *preferredNeighbor, bool activateVisPos) {

  // Allow interrupt here
  core->interruptAllowedHere();

  // If an abort has been requested, stop here
  if (abortUpdate)
    return;

  // Check if the area is plausible
  if (area.getYNorth()<area.getYSouth())
    return;
  if (area.getXEast()<area.getXWest())
    return;

  // Check if the maximum number of tiles to display are reached
  if (tiles.size()>=maxTiles)
    return;

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
  MapTile *tile=core->getMapSource()->findMapTileByGeographicArea(area,preferredNeighbor,container);

  // Tile found?
  Int searchedYNorth,searchedYSouth;
  Int searchedXWest,searchedXEast;
  double searchedLatNorth,searchedLatSouth,searchedLngEast,searchedLngWest;
  Int visXByCalibrator,visYByCalibrator;
  if (tile) {

    // Use the calibrator to compute the position
    MapPosition pos=area.getRefPos();
    tile->getParentMapContainer()->getMapCalibrator()->setPictureCoordinates(pos);
    Int diffVisX=tile->getMapX()-pos.getX();
    Int diffVisY=tile->getMapY()-pos.getY();
    visXByCalibrator=area.getRefPos().getX()+diffVisX;
    visYByCalibrator=area.getRefPos().getY()-diffVisY-tile->getHeight();

    // Has the tile already been processed?
    if (!tile->getIsProcessed()) {

      /* Some debug messages
      if (preferredNeighbor) {
        DEBUG("preferred neighbor: %s found tile: %s",preferredNeighbor->getVisName().front().c_str(),tile->getVisName().front().c_str());
      } else {
        DEBUG("found tile: %s",tile->getVisName().front().c_str());
      }*/

      // If the found tile is a direct neighbor: set the visual position accordingly
      bool useCalibratorPosition=true;
      if (preferredNeighbor) {
        if ((useCalibratorPosition)&&(preferredNeighbor->isNorthWestNeighbor(tile))) {
          tile->setVisX(preferredNeighbor->getVisX(0)-tile->getWidth());
          tile->setVisY(preferredNeighbor->getVisY(0)+tile->getHeight());
          useCalibratorPosition=false;
        }
        if ((useCalibratorPosition)&&(preferredNeighbor->isNorthNeighbor(tile))) {
          tile->setVisX(preferredNeighbor->getVisX(0));
          tile->setVisY(preferredNeighbor->getVisY(0)+tile->getHeight());
          useCalibratorPosition=false;
        }
        if ((useCalibratorPosition)&&(preferredNeighbor->isNorthEastNeighbor(tile))) {
          tile->setVisX(preferredNeighbor->getVisX(0)+preferredNeighbor->getWidth());
          tile->setVisY(preferredNeighbor->getVisY(0)+tile->getHeight());
          useCalibratorPosition=false;
        }
        if ((useCalibratorPosition)&&(preferredNeighbor->isEastNeighbor(tile))) {
          tile->setVisX(preferredNeighbor->getVisX(0)+preferredNeighbor->getWidth());
          tile->setVisY(preferredNeighbor->getVisY(0));
          useCalibratorPosition=false;
        }
        if ((useCalibratorPosition)&&(preferredNeighbor->isSouthEastNeighbor(tile))) {
          tile->setVisX(preferredNeighbor->getVisX(0)+preferredNeighbor->getWidth());
          tile->setVisY(preferredNeighbor->getVisY(0)-preferredNeighbor->getHeight());
          useCalibratorPosition=false;
        }
        if ((useCalibratorPosition)&&(preferredNeighbor->isSouthNeighbor(tile))) {
          tile->setVisX(preferredNeighbor->getVisX(0));
          tile->setVisY(preferredNeighbor->getVisY(0)-preferredNeighbor->getHeight());
          useCalibratorPosition=false;
        }
        if ((useCalibratorPosition)&&(preferredNeighbor->isSouthWestNeighbor(tile))) {
          tile->setVisX(preferredNeighbor->getVisX(0)-tile->getWidth());
          tile->setVisY(preferredNeighbor->getVisY(0)-preferredNeighbor->getHeight());
          useCalibratorPosition=false;
        }
        if ((useCalibratorPosition)&&(preferredNeighbor->isWestNeighbor(tile))) {
          tile->setVisX(preferredNeighbor->getVisX(0)-tile->getWidth());
          tile->setVisY(preferredNeighbor->getVisY(0));
          useCalibratorPosition=false;
        }
      }

      // Use the position computed by the calibrator if no direct neighbor
      if (useCalibratorPosition) {
        //DEBUG("using calibrator to compute position",NULL);
        MapPosition pos=area.getRefPos();
        tile->getParentMapContainer()->getMapCalibrator()->setPictureCoordinates(pos);
        Int diffVisX=tile->getMapX()-pos.getX();
        Int diffVisY=tile->getMapY()-pos.getY();
        tile->setVisX(visXByCalibrator);
        tile->setVisY(visYByCalibrator);
      }

      // Tile not in draw list?
      if (!tile->isDrawn()) {

        //DEBUG("tile is not visible,adding it to map",NULL);

        // Add the tile to the map
        tiles.push_back(tile);
        GraphicObject *v=tile->getVisualization();
        map->lockAccess();
        tile->setVisualizationKey(map->addPrimitive(v));
        map->unlockAccess();

        // Shall the position be activated immediately?
        if (activateVisPos) {

          // Tell the tile that it's visibility has changed
          tile->setIsHidden(false);

        } else {

          // Hide the tile until the search is over
          tile->setIsHidden(true);

        }

        /* For debugging
        if (core->getGraphicEngine()->getDebugMode()) {
          core->getGraphicEngine()->draw();
          DEBUG("redraw!",NULL);
        }*/

      }

      // Activate the new visual position?
      if (activateVisPos) {
        tile->getVisualization()->lockAccess();
        tile->activateVisPos();
        tile->getVisualization()->unlockAccess();
      }

      // Tile has been processed
      tile->setIsProcessed(true);
    }

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
    fillGeographicAreaWithTiles(nw,tile,activateVisPos);
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
    fillGeographicAreaWithTiles(n,tile,activateVisPos);
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
    fillGeographicAreaWithTiles(ne,tile,activateVisPos);
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
    fillGeographicAreaWithTiles(e,tile,activateVisPos);
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
    fillGeographicAreaWithTiles(se,tile,activateVisPos);
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
    fillGeographicAreaWithTiles(s,tile,activateVisPos);
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
    fillGeographicAreaWithTiles(sw,tile,activateVisPos);
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
    fillGeographicAreaWithTiles(w,tile,activateVisPos);
}

// Checks if a map update is required
bool MapEngine::mapUpdateIsRequired(GraphicPosition &visPos, Int *diffVisX, Int *diffVisY, double *diffZoom, bool checkLocationPos) {

  // Check if it is time to reset the visual position to the gps position
  if ((checkLocationPos)&&(returnToLocation)) {
    TimestampInMicroseconds diff=core->getClock()->getMicrosecondsSinceStart()-visPos.getLastUserModification();
    //DEBUG("locationPos2visPosOffsetValid=%d diff=%d returnToLocationTimeout=%d locationPos2visPosOffsetX=%d locationPos2visPosOffsetY=%d",locationPos2visPosOffsetValid,diff,returnToLocationTimeout,locationPos2visPosOffsetX,locationPos2visPosOffsetY);
    if ((diff>returnToLocationTimeout)||(returnToLocationOneTime)) {
      MapPosition newPos,oldPos;
      lockLocationPos();
      newPos=locationPos;
      unlockLocationPos();
      lockMapPos();
      oldPos=mapPos;
      unlockMapPos();
      if ((newPos.isValid())&&(mapPos.isValid())) {
        if (oldPos.getMapTile()) {
          double diffX = floor(fabs(newPos.getLng()-oldPos.getLng()) * oldPos.getMapTile()->getLngScale());
          double diffY = floor(fabs(newPos.getLat()-oldPos.getLat()) * oldPos.getMapTile()->getLatScale());
          if ((diffX>=1)||(diffY>=1)) {
            setMapPos(locationPos);
          }
        } else {
          if ((newPos.getLat()!=oldPos.getLat())||(newPos.getLng()!=oldPos.getLng())) {
            setMapPos(locationPos);
          }
        }
      }
    }
  }

  // Compute the offset to the previous position
  Int tmpDiffVisX=(visPos.getX()-this->visPos.getX());
  Int tmpDiffVisY=(visPos.getY()-this->visPos.getY());
  double tmpDiffZoom=visPos.getZoom()/this->visPos.getZoom();
  //DEBUG("diffVisX=%d diffVisY=%d",diffVisX,diffVisY);

  // Update the external variables
  if (diffVisX) *diffVisX=tmpDiffVisX;
  if (diffVisY) *diffVisY=tmpDiffVisY;
  if (diffZoom) *diffZoom=tmpDiffZoom;

  // Do not update map graphic if no change
  //DEBUG("diffVisX=%d diffVisY=%d diffZoom=%f",diffVisX,diffVisY,diffZoom);
  if ((tmpDiffVisX==0)&&(tmpDiffVisY==0)&&(tmpDiffZoom==1.0)&&(forceMapUpdate==false)&&(forceCacheUpdate==false)&&(forceMapRecreation==false)) {
    return false;
  } else {
    return true;
  }

}

// Sets the return to location flag
void MapEngine::setReturnToLocation(bool returnToLocation)
{
  if ((!this->returnToLocation)&&(returnToLocation)) {
    returnToLocationOneTime=true;
  }
  this->returnToLocation=returnToLocation;
  core->getConfigStore()->setIntValue("Map","returnToLocation",returnToLocation);
  if (returnToLocation) {
    INFO("return to location is enabled",NULL);
  } else {
    INFO("return to location is disabled",NULL);
  }
}

// Sets the zoom level lock flag
void MapEngine::setZoomLevelLock(bool zoomLevelLock)
{
  this->zoomLevelLock=zoomLevelLock;
  core->getConfigStore()->setIntValue("Map","zoomLevelLock",zoomLevelLock);
  if (zoomLevelLock) {
    INFO("zoom level lock is enabled",NULL);
  } else {
    INFO("zoom level lock is disabled",NULL);
  }
}

// Called when position of map has changed
void MapEngine::updateMap() {

  MapCalibrator *calibrator;
  Int diffVisX, diffVisY;
  double diffZoom;
  Int zoomLevel=0;
  bool mapChanged=false;

  PROFILE_START;

  // Check if object is initialized
  if (!isInitialized)
    return;

  // Indicate that the map is currently updated
  updateInProgress=true;

  // Check if a cache update is required
  if (forceCacheUpdate) {
    mapChanged=true;
    core->getThread()->lockMutex(forceCacheUpdateMutex);
    forceCacheUpdate=false;
    core->getThread()->unlockMutex(forceCacheUpdateMutex);
  }

  // Get the current position from the graphic engine
  GraphicPosition *visPos=core->getGraphicEngine()->lockPos();

  // Do not update map graphic if not required
  //DEBUG("diffVisX=%d diffVisY=%d diffZoom=%f",diffVisX,diffVisY,diffZoom);
  if (!mapUpdateIsRequired(*visPos,&diffVisX,&diffVisY,&diffZoom)) {
    core->getGraphicEngine()->unlockPos();
    PROFILE_ADD("no update required");
  } else {

    // Output some status
    //DEBUG("pos has changed to (%d,%d,%.2f,%.2f)",visPos->getX(),visPos->getY(),visPos->getZoom(),visPos->getAngle());

    // Do not return to location the next time
    returnToLocationOneTime=false;

    // Get the display area
    lockDisplayArea();
    MapArea displayArea=this->displayArea;
    unlockDisplayArea();

    // Use the offset to compute the new position in the map
    lockMapPos();
    MapPosition newMapPos;
    if (requestedMapPos.isValid()) {

      // First check if the new position lies within the previous display area
      // If it lies within the area, set the difference in pixels
      // If not, force a recreation of the map
      if (mapPos.getMapTile()) {
        if (mapPos.getMapTile()->getParentMapContainer()->getMapCalibrator()->setPictureCoordinates(requestedMapPos)) {
          //DEBUG("refX=%d refY=%d newX=%d newY=%d",mapPos.getX(),mapPos.getY(),requestedMapPos.getX(),requestedMapPos.getY());
          diffVisX=requestedMapPos.getX()-mapPos.getX();
          diffVisY=mapPos.getY()-requestedMapPos.getY();
          Int newX = displayArea.getRefPos().getX()+diffVisX;
          Int newY = displayArea.getRefPos().getY()-diffVisY;
          if (newX>displayArea.getXEast())
            forceMapRecreation=true;
          if (newX<displayArea.getXWest())
            forceMapRecreation=true;
          if (newY>displayArea.getYNorth())
            forceMapRecreation=true;
          if (newY<displayArea.getYSouth())
            forceMapRecreation=true;
        } else {
          forceMapRecreation=true;
        }
      } else {
        forceMapRecreation=true;
      }
      if (forceMapRecreation) {
        mapPos.setLng(requestedMapPos.getLng());
        mapPos.setLat(requestedMapPos.getLat());
        mapPos.setMapTile(NULL);
        diffVisX=0;
        diffVisY=0;
      } else {
        visPos->set(visPos->getX()+diffVisX,visPos->getY()+diffVisY,visPos->getZoom(),visPos->getAngle());
      }
      //DEBUG("diffVisX=%d diffVisY=%d lng=%f lat=%f forceMapRecreation=%d",diffVisX,diffVisY,mapPos.getLng(),mapPos.getLat(),forceMapRecreation);
      requestedMapPos.invalidate();
    }
    newMapPos=mapPos;
    unlockMapPos();
    newMapPos.setLngScale(newMapPos.getLngScale()*diffZoom);
    newMapPos.setLatScale(newMapPos.getLatScale()*diffZoom);

    // If the zoom has not changed, ensure that the last zoom level is used
    if ((diffZoom==1.0)||(zoomLevelLock)) {
      zoomLevel=displayArea.getZoomLevel();
    } else {
      zoomLevel=0;
    }

    // Check if visual position is near to overflow
    Int dX;
    if (visPos->getX()>=0) {
      dX=std::numeric_limits<Int>::max()-visPos->getX();
    } else {
      dX=visPos->getX()-std::numeric_limits<Int>::min();
    }
    Int dY;
    if (visPos->getY()>=0) {
      dY=std::numeric_limits<Int>::max()-visPos->getY();
    } else {
      dY=visPos->getY()-std::numeric_limits<Int>::min();
    }
    bool visPosResetted=false;
    if ((dY<initDistance)||(dX<initDistance)||(forceMapRecreation)) {
      visPos->set(0,0,visPos->getZoom(),visPos->getAngle());
      visPosResetted=true;
    }
    forceMapUpdate=false;
    forceMapRecreation=false;

    PROFILE_ADD("update init");

    // Find the map tile that closest matches the position
    // Lock the zoom level if the zoom did not change
    // Search all maps if no tile can be found for given zoom level
    core->getMapSource()->lockAccess();
    //DEBUG("lng=%f lat=%f",newMapPos.getLng(),newMapPos.getLat());
    MapTile *bestMapTile=core->getMapSource()->findMapTileByGeographicCoordinate(newMapPos,zoomLevel,zoomLevelLock);
    PROFILE_ADD("best map tile search");
    if (bestMapTile) {

      // Compute the new geo position
      calibrator=bestMapTile->getParentMapContainer()->getMapCalibrator();
      if ((!newMapPos.getMapTile())||(calibrator!=newMapPos.getMapTile()->getParentMapContainer()->getMapCalibrator()))
        calibrator->setPictureCoordinates(newMapPos);
      //DEBUG("bestMapTile=%08x",bestMapTile);
      //DEBUG("newMapPos.getX()=%d newMapPos.getY()=%d",newMapPos.getX(),newMapPos.getY());
      newMapPos.setMapTile(bestMapTile);
      newMapPos.setX(newMapPos.getX()+diffVisX);
      newMapPos.setY(newMapPos.getY()-diffVisY);
      calibrator->setGeographicCoordinates(newMapPos);
      //DEBUG("newMapPos.getX()=%d newMapPos.getY()=%d",newMapPos.getX(),newMapPos.getY());
      PROFILE_ADD("position update");

      // Check that there is a tile at the new geo position
      if (!core->getMapSource()->findMapTileByGeographicCoordinate(newMapPos,zoomLevel,zoomLevelLock)) {

        // Reset the visual position to the previous one
        //DEBUG("resetting visPos",NULL);
        lockMapPos();
        mapPos.setMapTile(NULL);
        unlockMapPos();
        *visPos=this->visPos;
        core->getGraphicEngine()->unlockPos();
        PROFILE_ADD("negative map tile existance check");

      } else {

        PROFILE_ADD("positive map tile existance check");

        // If the scale has changed, fill the complete screen with new tiles
        bool removeAllTiles=false;
        bool scaleHasChanged=false;
        double distToLngScale=fabs(bestMapTile->getLngScale()-displayArea.getRefPos().getLngScale());
        double distToLatScale=fabs(bestMapTile->getLatScale()-displayArea.getRefPos().getLatScale());
        if ((visPosResetted)||(bestMapTile->getParentMapContainer()->getZoomLevel()!=displayArea.getZoomLevel())) {

          // Ensure that all tiles are removed
          scaleHasChanged=true;
          removeAllTiles=true;

          /* Reset display area
          displayArea.setXEast(visPos->getX());
          displayArea.setXWest(visPos->getX());
          displayArea.setYNorth(visPos->getY());
          displayArea.setYSouth(visPos->getY());*/

        }

        // Compute the new zoom value for the found scale
        double newZoom;
        if (bestMapTile->getParentMapContainer()->getZoomLevel()!=displayArea.getZoomLevel()) {
          double newLngZoom=newMapPos.getLngScale()/bestMapTile->getLngScale();
          double newLatZoom=newMapPos.getLatScale()/bestMapTile->getLatScale();
          newZoom=(newLngZoom+newLatZoom)/2;
          visPos->setZoom(newZoom);
        } else {
          newZoom=visPos->getZoom();
        }

        // Compute the required display length
        //DEBUG("screenHeight=%d screenWidth=%d",core->getScreen()->getHeight(),core->getScreen()->getWidth());
        double alpha=atan((double)core->getScreen()->getHeight()/(double)core->getScreen()->getWidth());
        double screenLength=ceil(core->getScreen()->getHeight()/sin(alpha));

        // Compute the height and width to fill
        Int zoomedScreenHeight=ceil(screenLength/newZoom);
        Int zoomedScreenWidth=zoomedScreenHeight;
        //DEBUG("zoomedScreenHeight=%d zoomedScreenWidth=%d",zoomedScreenHeight,zoomedScreenWidth);

        // Compute the current boundaries that must be filled with tiles
        // Use the scale from the found map container
        MapArea newDisplayArea;
        MapPosition refPos=newMapPos;
        refPos.setX(visPos->getX());
        refPos.setY(visPos->getY());
        refPos.setLngScale(bestMapTile->getLngScale());
        refPos.setLatScale(bestMapTile->getLatScale());
        newDisplayArea.setRefPos(refPos);
        newDisplayArea.setZoomLevel(bestMapTile->getParentMapContainer()->getZoomLevel());
        newDisplayArea.setYNorth(refPos.getY()+zoomedScreenHeight/2);
        newDisplayArea.setYSouth(refPos.getY()-zoomedScreenHeight/2);
        newDisplayArea.setXWest(refPos.getX()-zoomedScreenWidth/2);
        newDisplayArea.setXEast(refPos.getX()+zoomedScreenWidth/2);

        // Compute the geographic boundaries
        MapPosition t=newMapPos;
        double latNorth,latSouth,lngWest,lngEast;
        t.setX(newMapPos.getX()-zoomedScreenWidth/2);
        t.setY(newMapPos.getY()-zoomedScreenHeight/2);
        calibrator->setGeographicCoordinates(t);
        latNorth=t.getLat();
        lngWest=t.getLng();
        t.setY(newMapPos.getY()+zoomedScreenHeight/2);
        calibrator->setGeographicCoordinates(t);
        latSouth=t.getLat();
        if (t.getLng()<lngWest) lngWest=t.getLng();
        t.setX(newMapPos.getX()+zoomedScreenWidth/2);
        calibrator->setGeographicCoordinates(t);
        if (t.getLat()<latSouth) latSouth=t.getLat();
        lngEast=t.getLng();
        t.setY(newMapPos.getY()-zoomedScreenHeight/2);
        calibrator->setGeographicCoordinates(t);
        if (t.getLat()>latNorth) latNorth=t.getLat();
        if (t.getLng()>lngEast) lngEast=t.getLng();
        newDisplayArea.setLatNorth(latNorth);
        newDisplayArea.setLatSouth(latSouth);
        newDisplayArea.setLngEast(lngEast);
        newDisplayArea.setLngWest(lngWest);

        // Remember the current visual position
        this->visPos=*visPos;
        core->getGraphicEngine()->unlockPos();
        lockMapPos();
        mapPos=newMapPos;
        unlockMapPos();

        // Set the new display area
        MapArea oldDisplayArea=displayArea;
        lockDisplayArea();
        this->displayArea=newDisplayArea;
        unlockDisplayArea();

        PROFILE_ADD("display area computation");

        // Update the overlaying graphic
        //DEBUG("before updateOverlays",NULL);
        core->getNavigationEngine()->updateScreenGraphic(scaleHasChanged);
        PROFILE_ADD("overlay graphic update");

        // Remove all tiles that are not visible anymore in the new display area
        std::list<MapTile*> visibleTiles=tiles;
        for (std::list<MapTile*>::const_iterator i=visibleTiles.begin();i!=visibleTiles.end();i++) {
          MapTile *t=*i;
          t->setIsProcessed(false);
          if ((removeAllTiles)||(t->getVisX(0)>newDisplayArea.getXEast())||(t->getVisX(1)<newDisplayArea.getXWest())||
          (t->getVisY(1)<newDisplayArea.getYSouth())||(t->getVisY(0)>newDisplayArea.getYNorth())) {
            //DEBUG("removing tile <%s> because it is out side the new display area",t->getVisName().c_str());
            tiles.remove(t);
            deinitTile(t);
          }
        }
        PROFILE_ADD("invisible tile remove");

        // Fill the complete area
        //DEBUG("starting area fill",NULL);
        MapArea searchArea=newDisplayArea;
        fillGeographicAreaWithTiles(searchArea,NULL,(tiles.size()==0) ? true : false);
        PROFILE_ADD("tile fill");

        // Update the access time of the tile and copy the visual position
        TimestampInSeconds currentTime=core->getClock()->getSecondsSinceEpoch();
        for (std::list<MapTile*>::const_iterator i=tiles.begin();i!=tiles.end();i++) {
          MapTile *t=*i;

          // Update last access time
          t->setLastAccess(currentTime);

          // Handle the tile position and visibility only if the process was not aborted
          if (!abortUpdate) {

            // Activate the new position
            t->getVisualization()->lockAccess();
            t->activateVisPos();
            t->getVisualization()->unlockAccess();

            // If the tile is hidden: make it visible
            if (t->getIsHidden()) {
              t->setIsHidden(false);
            }
          }
        }
        PROFILE_ADD("tile list update");

        // Request cache and map tile overlay graphic update
        mapChanged=true;
      }

    } else {

      // Reset the visual position to the previous one
      DEBUG("no map tile found",NULL);
      lockMapPos();
      mapPos.setMapTile(NULL);
      unlockMapPos();
      *visPos=this->visPos;
      core->getGraphicEngine()->unlockPos();
      PROFILE_ADD("no tile found");

    }
    core->getMapSource()->unlockAccess();

  }

  // Was the map changed?
  if (mapChanged) {

    // Update the tile graphic
    core->getNavigationEngine()->updateMapGraphic();

    // Inform the cache
    core->getMapCache()->tileVisibilityChanged();
  }

  // Indicate that the map update is finished
  updateInProgress=false;
  abortUpdate=false;
}

// Saves the current position
void MapEngine::backup() {

  // Only do the backup if we are initialized
  if (!isInitialized) {
    return;
  }

  // Create a copy of the position information
  lockMapPos();
  MapPosition mapPos=this->mapPos;
  unlockMapPos();
  lockDisplayArea();
  MapArea displayArea=this->displayArea;
  unlockDisplayArea();
  GraphicPosition visPos=*(core->getGraphicEngine()->lockPos());
  core->getGraphicEngine()->unlockPos();

  // Store the position
  core->getConfigStore()->setStringValue("Map/LastPosition","folder",core->getMapSource()->getFolder());
  core->getConfigStore()->setDoubleValue("Map/LastPosition","lng",mapPos.getLng());
  core->getConfigStore()->setDoubleValue("Map/LastPosition","lat",mapPos.getLat());
  core->getConfigStore()->setDoubleValue("Map/LastPosition","latScale",mapPos.getLatScale());
  core->getConfigStore()->setDoubleValue("Map/LastPosition","lngScale",mapPos.getLngScale());
  core->getConfigStore()->setDoubleValue("Map/LastPosition","zoom",visPos.getZoom());
  core->getConfigStore()->setDoubleValue("Map/LastPosition","angle",visPos.getAngle());
  core->getConfigStore()->setDoubleValue("Map/LastPosition","zoomLevel",displayArea.getZoomLevel());
}




}
