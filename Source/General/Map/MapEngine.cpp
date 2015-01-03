//============================================================================
// Name        : MapEngine.cpp
// Author      : Matthias Gruenewald
// Copyright   : Copyright 2010 Matthias Gruenewald
//
// This file is part of GeoDiscoverer.
//
// Unauthorized copying of this file, via any medium, is strictly prohibited.
// Proprietary and confidential.
//
//============================================================================

#include <Core.h>

namespace GEODISCOVERER {

// Constructor
MapEngine::MapEngine() {
  initDistance=core->getConfigStore()->getIntValue("Map","initDistance", __FILE__, __LINE__);
  maxTiles=0;
  returnToLocationTimeout=core->getConfigStore()->getIntValue("Map","returnToLocationTimeout", __FILE__, __LINE__);
  returnToLocationOneTime=false;
  returnToLocation=false;
  setReturnToLocation(core->getConfigStore()->getIntValue("Map","returnToLocation", __FILE__, __LINE__),false);
  setZoomLevelLock(core->getConfigStore()->getIntValue("Map","zoomLevelLock", __FILE__, __LINE__),false);
  map=NULL;
  updateInProgress=false;
  abortUpdate=false;
  forceMapUpdate=false;
  forceMapRecreation=false;
  forceCacheUpdate=false;
  locationPosMutex=core->getThread()->createMutex("map engine location pos mutex");
  mapPosMutex=core->getThread()->createMutex("map engine map pos mutex");
  compassBearingMutex=core->getThread()->createMutex("map engine compass bearing mutex");
  displayAreaMutex=core->getThread()->createMutex("map engine display area mutex");
  forceCacheUpdateMutex=core->getThread()->createMutex("map engine force cache update mutex");
  forceMapUpdateMutex=core->getThread()->createMutex("map engine force map update mutex");
  forceMapRedownloadMutex=core->getThread()->createMutex("map engine force map redownload mutex");
  prevCompassBearing=-std::numeric_limits<double>::max();
  compassBearing=prevCompassBearing;
  isInitialized=false;
  forceMapRedownload=false;
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
  core->getThread()->destroyMutex(forceMapRedownloadMutex);
}

// Does all action to remove a tile from the map
void MapEngine::deinitTile(MapTile *t, const char *file, int line)
{
  core->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
  map->removePrimitive(t->getVisualizationKey()); // Remove it from the graphic
  t->removeGraphic();
  if (t->getIsDummy())   // Delete dummy tiles
    delete t;
  core->getGraphicEngine()->unlockDrawing();
}

// Clear the current map
void MapEngine::deinitMap()
{
  // Deinit all tiles
  //DEBUG("deiniting tiles",NULL);
  for (std::list<MapTile*>::const_iterator i=tiles.begin();i!=tiles.end();i++) {
    MapTile *t=*i;
    t->setIsProcessed(false);
    deinitTile(t, __FILE__, __LINE__);
  }
  tiles.clear();

  // Delete all left-over primitives that were used for debugging
  //DEBUG("deleting debug primitives",NULL);
  if (map) {
    core->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
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
    core->getGraphicEngine()->unlockDrawing();
  }

  // Free graphic objects
  //DEBUG("deleting map",NULL);
  if (map) {
    core->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
    core->getGraphicEngine()->setMap(NULL);
    core->getGraphicEngine()->unlockDrawing();
    delete map;
    map=NULL;
  }

  // Object is not initialized anymore
  isInitialized=false;
}

// Initializes the map
void MapEngine::initMap()
{
  // Clear the current map
  deinitMap();

  // Get the current position from the graphic engine
  GraphicPosition *graphicEngineVisPos=core->getGraphicEngine()->lockPos(__FILE__, __LINE__);
  visPos=*graphicEngineVisPos;
  core->getGraphicEngine()->unlockPos();

  // Set the current position
  std::string lastMapFolder=core->getConfigStore()->getStringValue("Map/LastPosition","folder", __FILE__, __LINE__);
  //DEBUG("lastMapFolder=%s",lastMapFolder.c_str());
  if (lastMapFolder!="*unknown*") {
    lockMapPos(__FILE__, __LINE__);
    mapPos.setLng(core->getConfigStore()->getDoubleValue("Map/LastPosition","lng", __FILE__, __LINE__));
    mapPos.setLat(core->getConfigStore()->getDoubleValue("Map/LastPosition","lat", __FILE__, __LINE__));
    mapPos.setLatScale(core->getConfigStore()->getDoubleValue("Map/LastPosition","latScale", __FILE__, __LINE__));
    mapPos.setLngScale(core->getConfigStore()->getDoubleValue("Map/LastPosition","lngScale", __FILE__, __LINE__));
    unlockMapPos();
    core->getMapSource()->lockAccess(__FILE__,__LINE__);
    MapTile *tile=core->getMapSource()->findMapTileByGeographicCoordinate(mapPos,0,false,NULL);
    core->getMapSource()->unlockAccess();
    //DEBUG("tile=%08x",tile);
    if (tile==NULL) {
      DEBUG("could not find tile, using center position of map",NULL);
      lockMapPos(__FILE__, __LINE__);
      mapPos=*(core->getMapSource()->getCenterPosition());
      unlockMapPos();
      lastMapFolder="*unknown*";
      core->getConfigStore()->setStringValue("Map/LastPosition","folder",lastMapFolder, __FILE__, __LINE__);
    }
  } else {
    lockMapPos(__FILE__, __LINE__);
    mapPos=*(core->getMapSource()->getCenterPosition());
    unlockMapPos();
  }
  lockDisplayArea(__FILE__, __LINE__);
  displayArea.setZoomLevel(0);
  unlockDisplayArea();
  if (lastMapFolder==core->getMapSource()->getFolder()) {
    visPos.setZoom(core->getConfigStore()->getDoubleValue("Map/LastPosition","zoom", __FILE__, __LINE__));
    visPos.setAngle(core->getConfigStore()->getDoubleValue("Map/LastPosition","angle", __FILE__, __LINE__));
    lockDisplayArea(__FILE__, __LINE__);
    displayArea.setZoomLevel(core->getConfigStore()->getIntValue("Map/LastPosition","zoomLevel", __FILE__, __LINE__));
    unlockDisplayArea();
  }

  // Store the new values
  backup();

  // Update the visual position in the graphic engine
  graphicEngineVisPos=core->getGraphicEngine()->lockPos(__FILE__, __LINE__);
  *graphicEngineVisPos=visPos;
  core->getGraphicEngine()->unlockPos();

  // Create new graphic objects
  map=new GraphicObject();
  if (!map) {
    FATAL("can not create graphic object for the map",NULL);
    return;
  }

  // Inform the graphic engine about the new objects
  core->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
  core->getGraphicEngine()->setMap(map);
  core->getGraphicEngine()->unlockDrawing();

  // Force redraw
  forceMapRecreation=true;

  // Object is initialized
  isInitialized=true;
}

// Remove all debugging primitives
void MapEngine::removeDebugPrimitives() {
  core->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
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
  core->getGraphicEngine()->unlockDrawing();
}

// Fills the given area with tiles
void MapEngine::fillGeographicAreaWithTiles(MapArea area, MapTile *preferredNeighbor, bool activateVisPos) {

  // If an abort has been requested, stop here
  if (abortUpdate) {
    //DEBUG("update aborted",NULL);
    return;
  }

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
  if (tiles.size()>=maxTiles) {
    //DEBUG("too many tiles",NULL);
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
  MapTile *tile=core->getMapSource()->findMapTileByGeographicArea(area,preferredNeighbor,container);
  //DEBUG("tile=0x%08x",tile);

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
        tile->setVisX(visXByCalibrator);
        tile->setVisY(visYByCalibrator);
      }

      // Remember this tile if it is in the direct neighborhood of the center position
      if ((
           (tile->getVisX(0)<area.getRefPos().getX()+core->getMapSource()->getMapTileWidth())&&
           (tile->getVisX(1)>area.getRefPos().getX()-core->getMapSource()->getMapTileWidth())
          )&&(
           (tile->getVisY(0)<area.getRefPos().getY()+core->getMapSource()->getMapTileHeight())&&
           (tile->getVisY(1)>area.getRefPos().getY()-core->getMapSource()->getMapTileHeight())
         ))
      {
        centerMapTiles.push_back(tile);
      }

      // Tile not in draw list?
      if (!tile->isDrawn()) {

        //DEBUG("tile is not visible,adding it to map",NULL);

        // Add the tile to the map
        tiles.push_back(tile);
        GraphicObject *v=tile->getVisualization();
        core->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
        tile->setVisualizationKey(map->addPrimitive(v));
        core->getGraphicEngine()->unlockDrawing();

        // Shall the position be activated immediately?
        if (activateVisPos) {

          // Tell the tile that it's visibility has changed
          tile->setIsHidden(false,__FILE__, __LINE__);

        } else {

          // Hide the tile until the search is over
          tile->setIsHidden(true,__FILE__, __LINE__);

        }

        /* For debugging
        if (core->getGraphicEngine()->getDebugMode()) {
          core->getGraphicEngine()->draw();
          DEBUG("redraw!",NULL);
        }*/

      }

      // Activate the new visual position?
      if (activateVisPos) {
        core->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
        tile->activateVisPos();
        core->getGraphicEngine()->unlockDrawing();
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
bool MapEngine::mapUpdateIsRequired(GraphicPosition visPos, Int *diffVisX, Int *diffVisY, double *diffZoom, bool checkLocationPos) {

  // Check if it is time to reset the visual position to the gps position
  if ((checkLocationPos)&&(returnToLocation)) {
    TimestampInMicroseconds diff=core->getClock()->getMicrosecondsSinceStart()-visPos.getLastUserModification();
    //DEBUG("locationPos2visPosOffsetValid=%d diff=%d returnToLocationTimeout=%d locationPos2visPosOffsetX=%d locationPos2visPosOffsetY=%d",locationPos2visPosOffsetValid,diff,returnToLocationTimeout,locationPos2visPosOffsetX,locationPos2visPosOffsetY);
    if ((diff>returnToLocationTimeout)||(returnToLocationOneTime)) {
      MapPosition newPos,oldPos;
      lockLocationPos(__FILE__, __LINE__);
      newPos=locationPos;
      unlockLocationPos();
      lockMapPos(__FILE__, __LINE__);
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
void MapEngine::setReturnToLocation(bool returnToLocation, bool showInfo)
{
  if ((!this->returnToLocation)&&(returnToLocation)) {
    returnToLocationOneTime=true;
  }
  this->returnToLocation=returnToLocation;
  core->getConfigStore()->setIntValue("Map","returnToLocation",returnToLocation, __FILE__, __LINE__);
  if (showInfo) {
    if (returnToLocation) {
      INFO("return to location is enabled",NULL);
    } else {
      INFO("return to location is disabled",NULL);
    }
  }
}

// Sets the zoom level lock flag
void MapEngine::setZoomLevelLock(bool zoomLevelLock, bool showInfo)
{
  this->zoomLevelLock=zoomLevelLock;
  core->getConfigStore()->setIntValue("Map","zoomLevelLock",zoomLevelLock,__FILE__, __LINE__);
  if (showInfo) {
    if (zoomLevelLock) {
      INFO("zoom level lock is enabled",NULL);
    } else {
      INFO("zoom level lock is disabled",NULL);
    }
  }
}

// Called when position of map has changed
void MapEngine::updateMap() {

  MapCalibrator *calibrator;
  Int diffVisX, diffVisY;
  double diffZoom;
  Int zoomLevel=0;
  bool mapChanged=false;

  //PROFILE_START;

  // Check if object is initialized
  if (!isInitialized)
    return;

  // Indicate that the map is currently updated
  updateInProgress=true;

  // Check if the tile images shall be updated
  if (forceMapRedownload) {
    MapDownloader *mapDownloader=core->getMapSource()->getMapDownloader();
    if (mapDownloader) {

      // Remove the visible tiles and remember the map container
      std::list<MapContainer*> visibleMapContainers;
      while (tiles.size()>0) {
        MapTile *t=tiles.back();
        bool found=false;
        for (std::list<MapContainer*>::iterator j=visibleMapContainers.begin();j!=visibleMapContainers.end();j++) {
          if (*j==t->getParentMapContainer()) {
            found=true;
            break;
          }
        }
        if (t->getParentMapContainer()->getDownloadComplete()) {
          if (!found) {
            visibleMapContainers.push_back(t->getParentMapContainer());
          }
          deinitTile(t, __FILE__, __LINE__);
          tiles.pop_back();
        }
      }

      // Tell the map source that the map containers are obsolete and shall also be deleted from the gda archive
      MapSource *mapSource = core->getMapSource();
      mapSource->lockAccess(__FILE__, __LINE__);
      for (std::list<MapContainer*>::iterator j=visibleMapContainers.begin();j!=visibleMapContainers.end();j++) {
        mapSource->markMapContainerObsolete(*j);
      }
      mapSource->unlockAccess();

      // Finally remove the map containers
      mapSource->removeObsoleteMapContainers(true);

      // Map has changed
      mapChanged=true;
    }
    core->getThread()->lockMutex(forceMapRedownloadMutex,__FILE__, __LINE__);
    forceMapRedownload=false;
    core->getThread()->unlockMutex(forceMapRedownloadMutex);
  }

  // Check if a cache update is required
  if (forceCacheUpdate) {
    mapChanged=true;
    core->getThread()->lockMutex(forceCacheUpdateMutex,__FILE__, __LINE__);
    forceCacheUpdate=false;
    core->getThread()->unlockMutex(forceCacheUpdateMutex);
  }


  // Get the current position from the graphic engine
  GraphicPosition *visPos=core->getGraphicEngine()->lockPos(__FILE__, __LINE__);

  // Do not update map graphic if not required
  //DEBUG("diffVisX=%d diffVisY=%d diffZoom=%f",diffVisX,diffVisY,diffZoom);
  if (!mapUpdateIsRequired(*visPos,&diffVisX,&diffVisY,&diffZoom)) {
    core->getGraphicEngine()->unlockPos();
    //PROFILE_ADD("no update required");
  } else {

    // Output some status
    //DEBUG("pos has changed to (%d,%d,%.2f,%.2f)",visPos->getX(),visPos->getY(),visPos->getZoom(),visPos->getAngle());

    // Do not return to location the next time
    returnToLocationOneTime=false;

    // Get the display area
    lockDisplayArea(__FILE__, __LINE__);
    MapArea displayArea=this->displayArea;
    unlockDisplayArea();

    // Use the offset to compute the new position in the map
    lockMapPos(__FILE__, __LINE__);
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
    core->getThread()->lockMutex(forceMapUpdateMutex, __FILE__, __LINE__);
    forceMapUpdate=false;
    core->getThread()->unlockMutex(forceMapUpdateMutex);
    forceMapRecreation=false;

    //PROFILE_ADD("update init");

    // Find the map tile that closest matches the position
    // Lock the zoom level if the zoom did not change
    // Search all maps if no tile can be found for given zoom level
    core->getMapSource()->lockAccess(__FILE__,__LINE__);
    //DEBUG("lng=%f lat=%f",newMapPos.getLng(),newMapPos.getLat());
    MapTile *bestMapTile=core->getMapSource()->findMapTileByGeographicCoordinate(newMapPos,zoomLevel,zoomLevelLock);
    //PROFILE_ADD("best map tile search");
    //DEBUG("bestMapTile=%08x",bestMapTile);
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
      //PROFILE_ADD("position update");

      // Check that there is a tile at the new geo position
      if (!core->getMapSource()->findMapTileByGeographicCoordinate(newMapPos,zoomLevel,zoomLevelLock)) {

        // Reset the visual position to the previous one
        //DEBUG("resetting visPos",NULL);
        lockMapPos(__FILE__, __LINE__);
        mapPos.setMapTile(NULL);
        unlockMapPos();
        *visPos=this->visPos;
        core->getGraphicEngine()->unlockPos();
        //PROFILE_ADD("negative map tile existance check");

      } else {

        //PROFILE_ADD("positive map tile existance check");

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
          //DEBUG("zoom level changed",NULL);
        } else {
          newZoom=visPos->getZoom();
          //DEBUG("zoom level not changed",NULL);
        }
        //DEBUG("newZoom=%f",newZoom);

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
        /*DEBUG("latNorth=%f latSouth=%f lngEast=%f lngWest=%f",
              newDisplayArea.getLatNorth(),newDisplayArea.getLatSouth(),
              newDisplayArea.getLngEast(),newDisplayArea.getLngWest());*/

        // Remember the current visual position
        this->visPos=*visPos;
        core->getGraphicEngine()->unlockPos();
        lockMapPos(__FILE__, __LINE__);
        mapPos=newMapPos;
        unlockMapPos();

        // Set the new display area
        MapArea oldDisplayArea=displayArea;
        lockDisplayArea(__FILE__, __LINE__);
        this->displayArea=newDisplayArea;
        unlockDisplayArea();

        //PROFILE_ADD("display area computation");

        // Update the overlaying graphic
        //DEBUG("before updateOverlays",NULL);
        core->getNavigationEngine()->updateScreenGraphic(scaleHasChanged);
        //PROFILE_ADD("overlay graphic update");

        // Remove all tiles that are not visible anymore in the new display area
        std::list<MapTile*> visibleTiles=tiles;
        for (std::list<MapTile*>::const_iterator i=visibleTiles.begin();i!=visibleTiles.end();i++) {
          MapTile *t=*i;
          t->setIsProcessed(false);
          if ((removeAllTiles)||(t->getVisX(0)>newDisplayArea.getXEast())||(t->getVisX(1)<newDisplayArea.getXWest())||
          (t->getVisY(1)<newDisplayArea.getYSouth())||(t->getVisY(0)>newDisplayArea.getYNorth())) {
            //DEBUG("removing tile <%s> because it is out side the new display area",t->getVisName().c_str());
            tiles.remove(t);
            deinitTile(t, __FILE__, __LINE__);
          }
        }
        //PROFILE_ADD("invisible tile remove");

        // Fill the complete area
        //DEBUG("starting area fill",NULL);
        MapArea searchArea=newDisplayArea;
        centerMapTiles.clear();
        core->interruptAllowedHere(__FILE__, __LINE__);
        fillGeographicAreaWithTiles(searchArea,NULL,(tiles.size()==0) ? true : false);
        //PROFILE_ADD("tile fill");

        // Update the access time of the tile and copy the visual position
        TimestampInSeconds currentTime=core->getClock()->getSecondsSinceEpoch();
        for (std::list<MapTile*>::const_iterator i=tiles.begin();i!=tiles.end();i++) {
          MapTile *t=*i;

          // Update last access time
          t->setLastAccess(currentTime);

          // Handle the tile position and visibility only if the process was not aborted
          if (!abortUpdate) {

            // Activate the new position
            core->getGraphicEngine()->lockDrawing(__FILE__, __LINE__);
            t->activateVisPos();
            core->getGraphicEngine()->unlockDrawing();

            // If the tile is hidden: make it visible
            if (t->getIsHidden()) {
              t->setIsHidden(false,__FILE__, __LINE__);
            }
          }
        }
        //PROFILE_ADD("tile list update");

        // If no tile has been found for whatever reason, reset the map
        //DEBUG("tiles.size()=%d zoomedScreenWidth=%d",tiles.size(),zoomedScreenWidth);
        if ((!abortUpdate)&&((tiles.size()==0)||(zoomedScreenWidth<=1))) {
        	DEBUG("resetting map",NULL);
          core->getConfigStore()->setStringValue("Map/LastPosition","folder","*unknown*",__FILE__, __LINE__);
          initMap();
        }

        // Request cache and map tile overlay graphic update
        mapChanged=true;
      }

    } else {

      // Reset the visual position to the previous one
      DEBUG("no map tile found",NULL);
      lockMapPos(__FILE__, __LINE__);
      mapPos.setMapTile(NULL);
      unlockMapPos();
      *visPos=this->visPos;
      core->getGraphicEngine()->unlockPos();
      //PROFILE_ADD("no tile found");

    }
    core->getMapSource()->unlockAccess();

  }

  // Was the map changed?
  if (mapChanged) {

    // Update the tile graphic
    core->getNavigationEngine()->updateMapGraphic();

    // Update any widgets
    core->getWidgetEngine()->onMapChange(mapPos,&centerMapTiles);

    // Inform the cache
    core->getMapCache()->tileVisibilityChanged();
  }

  // Indicate that the map update is finished
  updateInProgress=false;
  if (abortUpdate)
    setForceMapUpdate(__FILE__,__LINE__);
  abortUpdate=false;
}

// Saves the current position
void MapEngine::backup() {

  // Only do the backup if we are initialized
  if (!isInitialized) {
    return;
  }

  // Create a copy of the position information
  lockMapPos(__FILE__, __LINE__);
  MapPosition mapPos=this->mapPos;
  unlockMapPos();
  lockDisplayArea(__FILE__, __LINE__);
  MapArea displayArea=this->displayArea;
  unlockDisplayArea();
  GraphicPosition visPos=*(core->getGraphicEngine()->lockPos(__FILE__, __LINE__));
  core->getGraphicEngine()->unlockPos();

  // Store the position
  core->getConfigStore()->setStringValue("Map/LastPosition","folder",core->getMapSource()->getFolder(), __FILE__, __LINE__);
  core->getConfigStore()->setDoubleValue("Map/LastPosition","lng",mapPos.getLng(), __FILE__, __LINE__);
  core->getConfigStore()->setDoubleValue("Map/LastPosition","lat",mapPos.getLat(), __FILE__, __LINE__);
  core->getConfigStore()->setDoubleValue("Map/LastPosition","latScale",mapPos.getLatScale(), __FILE__, __LINE__);
  core->getConfigStore()->setDoubleValue("Map/LastPosition","lngScale",mapPos.getLngScale(), __FILE__, __LINE__);
  core->getConfigStore()->setDoubleValue("Map/LastPosition","zoom",visPos.getZoom(), __FILE__, __LINE__);
  core->getConfigStore()->setDoubleValue("Map/LastPosition","angle",visPos.getAngle(), __FILE__, __LINE__);
  core->getConfigStore()->setDoubleValue("Map/LastPosition","zoomLevel",displayArea.getZoomLevel(), __FILE__, __LINE__);
}

// Sets the maximum number of tiles to show
void MapEngine::setMaxTiles() {
  int len=core->getScreen()->getHeight();
  if (core->getScreen()->getWidth()>len)
    len=core->getScreen()->getWidth();
  maxTiles=ceil(((double)len)/((double)core->getMapSource()->getMapTileLength())*((double)core->getConfigStore()->getIntValue("Map","visibleTileLimit", __FILE__, __LINE__)));
  maxTiles=maxTiles*maxTiles;
}

// Sets a new map position
void MapEngine::setMapPos(MapPosition mapPos)
{
  bool updateMap = false;
  core->getMapSource()->lockAccess(__FILE__,__LINE__);
  MapTile *tile=core->getMapSource()->findMapTileByGeographicCoordinate(mapPos,0,false,NULL);
  core->getMapSource()->unlockAccess();
  if (tile) {
    core->getThread()->lockMutex(mapPosMutex, __FILE__, __LINE__);
    requestedMapPos = mapPos;
    core->getThread()->unlockMutex(mapPosMutex);
    setForceMapUpdate(__FILE__, __LINE__);
  } else {
    DEBUG("requested map pos has been ignored because map contains no tile",NULL);
  }
}


}
