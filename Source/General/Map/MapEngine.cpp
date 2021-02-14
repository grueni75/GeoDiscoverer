//============================================================================
// Name        : MapEngine.cpp
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
#include <MapEngine.h>
#include <GraphicEngine.h>
#include <MapTile.h>
#include <MapSource.h>
#include <Device.h>
#include <NavigationEngine.h>

namespace GEODISCOVERER {

// Constructor
MapEngine::MapEngine() {
  initDistance=core->getConfigStore()->getIntValue("Map","initDistance", __FILE__, __LINE__);
  tileOffScreenFactor=core->getConfigStore()->getIntValue("Map","tileOffScreenFactor", __FILE__, __LINE__);
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
  forceZoomReset=false;
  forceCacheUpdate=false;
  locationPosMutex=core->getThread()->createMutex("map engine location pos mutex");
  mapPosMutex=core->getThread()->createMutex("map engine map pos mutex");
  compassBearingMutex=core->getThread()->createMutex("map engine compass bearing mutex");
  displayAreaMutex=core->getThread()->createMutex("map engine display area mutex");
  forceCacheUpdateMutex=core->getThread()->createMutex("map engine force cache update mutex");
  forceMapUpdateMutex=core->getThread()->createMutex("map engine force map update mutex");
  forceMapRedownloadMutex=core->getThread()->createMutex("map engine force map redownload mutex");
  centerMapTilesMutex=core->getThread()->createMutex("map engine center map tiles mutex");
  prevCompassBearing=-std::numeric_limits<double>::max();
  compassBearing=prevCompassBearing;
  isInitialized=false;
  forceMapRedownload=false;
  redownloadAllZoomLevels=false;
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
  core->getThread()->destroyMutex(centerMapTilesMutex);
}

// Does all action to remove a tile from the map
void MapEngine::deinitTile(MapTile *t, const char *file, int line)
{
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
  map->removePrimitive(t->getVisualizationKey()); // Remove it from the graphic
  t->removeGraphic();
  if (t->getIsDummy())   // Delete dummy tiles
    delete t;
  core->getDefaultGraphicEngine()->unlockDrawing();
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
    core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
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
    core->getDefaultGraphicEngine()->unlockDrawing();
  }

  // Free graphic objects
  //DEBUG("deleting map",NULL);
  if (map) {
    core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
    core->getDefaultGraphicEngine()->setMap(NULL);
    core->getDefaultGraphicEngine()->unlockDrawing();
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
  GraphicPosition *graphicEngineVisPos=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
  visPos=*graphicEngineVisPos;
  core->getDefaultGraphicEngine()->unlockPos();

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
  graphicEngineVisPos=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
  *graphicEngineVisPos=visPos;
  core->getDefaultGraphicEngine()->unlockPos();

  // Create new graphic objects
  map=new GraphicObject(core->getDefaultScreen());
  if (!map) {
    FATAL("can not create graphic object for the map",NULL);
    return;
  }

  // Inform the graphic engine about the new objects
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
  core->getDefaultGraphicEngine()->setMap(map);
  core->getDefaultGraphicEngine()->unlockDrawing();

  // Force redraw
  forceMapRecreation=true;

  // Object is initialized
  isInitialized=true;
}

// Remove all debugging primitives
void MapEngine::removeDebugPrimitives() {
  core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
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
  core->getDefaultGraphicEngine()->unlockDrawing();
}

// Fills the given area with tiles
void MapEngine::fillGeographicAreaWithTiles(MapArea area, MapTile *preferredNeighbor, bool activateVisPos) {

  std::list<MapTile*> foundTiles;

  // Get the list of tiles in the area
  core->getMapSource()->fillGeographicAreaWithTiles(area, preferredNeighbor, maxTiles, &foundTiles);

  // Go through all found ones
  for (std::list<MapTile*>::iterator i=foundTiles.begin();i!=foundTiles.end();i++) {

    MapTile *tile=*i;;
    //Int searchedYNorth,searchedYSouth;
    //Int searchedXWest,searchedXEast;
    //double searchedLatNorth,searchedLatSouth,searchedLngEast,searchedLngWest;
    Int visXByCalibrator,visYByCalibrator;

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
      /*if (preferredNeighbor) {
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
      }*/

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
        lockCenterMapTiles(__FILE__,__LINE__);
        centerMapTiles.push_back(tile);
        unlockCenterMapTiles();
      }

      // Tile not in draw list?
      if (!tile->isDrawn()) {

        //DEBUG("tile is not visible,adding it to map",NULL);

        // Add the tile to the map
        tiles.push_back(tile);
        GraphicObject *v=tile->getVisualization();
        core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
        tile->setVisualizationKey(map->addPrimitive(v));
        core->getDefaultGraphicEngine()->unlockDrawing();

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
        core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
        tile->activateVisPos();
        core->getDefaultGraphicEngine()->unlockDrawing();
      }

      // Tile has been processed
      tile->setIsProcessed(true);
    }
  }
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
  if ((tmpDiffVisX==0)&&(tmpDiffVisY==0)&&(tmpDiffZoom==1.0)&&(forceMapUpdate==false)&&(forceCacheUpdate==false)&&(forceMapRecreation==false)&&(forceZoomReset==false)) {
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
  if ((showInfo)&&(!core->getDefaultDevice()->getIsWatch())) {
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
  if ((showInfo)&&(!core->getDefaultDevice()->getIsWatch())) {
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
  GraphicPosition visPosRO,*visPosRW;
  
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

      // Open a busy dialog
      DialogKey busyDialog = core->getDialog()->createProgress("Cleaning map archives",0);

      // Remove the visible tiles from the map engine
      while (tiles.size()>0) {
        MapTile *t=tiles.back();
        if (t->getParentMapContainer()->getDownloadComplete()) {
          lockMapPos(__FILE__, __LINE__);
          if (mapPos.getMapTile()==t) {
            mapPos.setMapTile(NULL);
          }
          unlockMapPos();
          deinitTile(t, __FILE__, __LINE__);
        }
        tiles.pop_back();
      }

      // Mark map containers obsolete depending on the requested zoom level to clear
      lockDisplayArea(__FILE__, __LINE__);
      MapArea displayArea=this->displayArea;
      unlockDisplayArea();
      MapSource *mapSource = core->getMapSource();
      mapSource->lockAccess(__FILE__, __LINE__);
      std::vector<MapContainer*> mapContainers=*(mapSource->getMapContainers());
      for (std::vector<MapContainer*>::iterator i=mapContainers.begin();i!=mapContainers.end();i++) {
        MapContainer *c=*i;
        if ((!redownloadAllZoomLevels)&&(c->getZoomLevelMap()!=displayArea.getZoomLevel()))
          c=NULL;
        if ((c)&&(c->getDownloadComplete())&&
        (c->getLatSouth()<=displayArea.getLatNorth())&&(c->getLatNorth()>=displayArea.getLatSouth())&&
        (c->getLngWest()<=displayArea.getLngEast())&&(c->getLngEast()>=displayArea.getLngWest())) {
          mapSource->markMapContainerObsolete(c);
        }
      }
      mapSource->unlockAccess();

      // Finally remove the map containers
      mapSource->removeObsoleteMapContainers(&displayArea,redownloadAllZoomLevels);

      // Close busy dialog
      core->getDialog()->closeProgress(busyDialog);

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
  visPosRO=*core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
  core->getDefaultGraphicEngine()->unlockPos();

  // Do not update map graphic if not required
  //DEBUG("diffVisX=%d diffVisY=%d diffZoom=%f",diffVisX,diffVisY,diffZoom);
  if (!mapUpdateIsRequired(visPosRO,&diffVisX,&diffVisY,&diffZoom)) {
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
    newMapPos=mapPos;
    if (requestedMapPos.isValid()) {

      // First check if the new position lies within the previous display area
      // If it lies within the area, set the difference in pixels
      // If not, force a recreation of the map
      if (mapPos.getMapTile()) {
        //DEBUG("map tile found",NULL);
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
          //DEBUG("forceMapCreation=%d",forceMapRecreation);
        } else {
          forceMapRecreation=true;
        }
      } else {
        //DEBUG("no map tile found",NULL);
        forceMapRecreation=true;
      }
      if (forceMapRecreation) {
        newMapPos.setLng(requestedMapPos.getLng());
        newMapPos.setLat(requestedMapPos.getLat());
        newMapPos.setMapTile(NULL);
        diffVisX=0;
        diffVisY=0;
      } else {
        visPosRW=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
        visPosRW->set(visPosRW->getX()+diffVisX,visPosRW->getY()+diffVisY,visPosRW->getZoom(),visPosRW->getAngle());
        core->getDefaultGraphicEngine()->unlockPos();
      }
      //DEBUG("diffVisX=%d diffVisY=%d lng=%f lat=%f forceMapRecreation=%d",diffVisX,diffVisY,mapPos.getLng(),mapPos.getLat(),forceMapRecreation);
      requestedMapPos.invalidate();
    }
    unlockMapPos();
    newMapPos.setLngScale(newMapPos.getLngScale()*diffZoom);
    newMapPos.setLatScale(newMapPos.getLatScale()*diffZoom);

    // If the zoom has not changed, ensure that the last zoom level is used
    if (((!forceMapRecreation)&&(!forceZoomReset)&&(diffZoom==1.0))||(zoomLevelLock)) {
      zoomLevel=displayArea.getZoomLevel();
    } else {
      zoomLevel=0;
    }
    forceZoomReset=false;
    //DEBUG("diffZoom=%f zoomLevelLock=%d zoomLevel=%d",diffZoom,zoomLevelLock,zoomLevel);

    // Check if visual position is near to overflow
    visPosRW=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
    Int dX;
    if (visPosRW->getX()>=0) {
      dX=std::numeric_limits<Int>::max()-visPosRW->getX();
    } else {
      dX=visPosRW->getX()-std::numeric_limits<Int>::min();
    }
    Int dY;
    if (visPosRW->getY()>=0) {
      dY=std::numeric_limits<Int>::max()-visPosRW->getY();
    } else {
      dY=visPosRW->getY()-std::numeric_limits<Int>::min();
    }
    bool visPosResetted=false;
    if ((dY<initDistance)||(dX<initDistance)||(forceMapRecreation)) {
      visPosRW->set(0,0,visPosRW->getZoom(),visPosRW->getAngle());
      visPosResetted=true;
    }
    core->getDefaultGraphicEngine()->unlockPos();
    core->getThread()->lockMutex(forceMapUpdateMutex, __FILE__, __LINE__);
    forceMapUpdate=false;
    core->getThread()->unlockMutex(forceMapUpdateMutex);
    if (forceMapRecreation)
      forceMapRecreation=false;

    //PROFILE_ADD("update init");

    // Find the map tile that closest matches the position
    // Lock the zoom level if the zoom did not change
    // Search all maps if no tile can be found for given zoom level
    core->getMapSource()->lockAccess(__FILE__,__LINE__);
    //DEBUG("lng=%f lat=%f",newMapPos.getLng(),newMapPos.getLat());
    //DEBUG("zoomLevel=%d",zoomLevel);
    MapTile *bestMapTile=core->getMapSource()->findMapTileByGeographicCoordinate(newMapPos,zoomLevel,zoomLevelLock);
    //DEBUG("bestMapTile->getZoomLevelMap()=%d",bestMapTile->getParentMapContainer()->getZoomLevelMap());
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
        visPosRW=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
        *visPosRW=this->visPos;
        core->getDefaultGraphicEngine()->unlockPos();
        //PROFILE_ADD("negative map tile existance check");

      } else {

        //PROFILE_ADD("positive map tile existance check");

        // If the scale has changed, fill the complete screen with new tiles
        bool removeAllTiles=false;
        bool scaleHasChanged=false;
        double distToLngScale=fabs(bestMapTile->getLngScale()-displayArea.getRefPos().getLngScale());
        double distToLatScale=fabs(bestMapTile->getLatScale()-displayArea.getRefPos().getLatScale());
        if ((visPosResetted)||(bestMapTile->getParentMapContainer()->getZoomLevelMap()!=displayArea.getZoomLevel())) {

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
        visPosRW=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
        if (bestMapTile->getParentMapContainer()->getZoomLevelMap()!=displayArea.getZoomLevel()) {
          double newLngZoom=newMapPos.getLngScale()/bestMapTile->getLngScale();
          double newLatZoom=newMapPos.getLatScale()/bestMapTile->getLatScale();
          newZoom=(newLngZoom+newLatZoom)/2;
          visPosRW->setZoom(newZoom);
          //DEBUG("zoom level changed",NULL);
        } else {
          newZoom=visPosRW->getZoom();
          //DEBUG("zoom level not changed",NULL);
        }
        core->getDefaultGraphicEngine()->unlockPos();
        //DEBUG("newZoom=%f",newZoom);

        // Compute the required display length
        //DEBUG("screenHeight=%d screenWidth=%d",core->getScreen()->getHeight(),core->getScreen()->getWidth());
        double alpha=atan((double)core->getDefaultScreen()->getHeight()/(double)core->getDefaultScreen()->getWidth());
        double screenLength=ceil(core->getDefaultScreen()->getHeight()/sin(alpha))*tileOffScreenFactor;
        //DEBUG("screenWidth=%d screenHeight=%d screenLength=%f",core->getDefaultScreen()->getWidth(),core->getDefaultScreen()->getHeight(),screenLength);

        // Compute the height and width to fill
        double screenScale = core->getDefaultGraphicEngine()->getMapTileToScreenScale(core->getDefaultScreen());
        Int zoomedScreenHeight=ceil(screenLength/(newZoom*screenScale));
        Int zoomedScreenWidth=zoomedScreenHeight;
        //DEBUG("zoomedScreenHeight=%d zoomedScreenWidth=%d",zoomedScreenHeight,zoomedScreenWidth);

        // Abort if the screen width is unreasonable
        if (zoomedScreenWidth<=1) {

          visPosRW=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
          *visPosRW=this->visPos;
          core->getDefaultGraphicEngine()->unlockPos();

        } else {

          // Compute the current boundaries that must be filled with tiles
          // Use the scale from the found map container
          MapArea newDisplayArea;
          MapPosition refPos=newMapPos;
          visPosRW=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
          refPos.setX(visPosRW->getX());
          refPos.setY(visPosRW->getY());
          core->getDefaultGraphicEngine()->unlockPos();
          //DEBUG("refPos.getX()=%d refPos.getY()=%d",refPos.getX(),refPos.getY());
          refPos.setLngScale(bestMapTile->getLngScale());
          refPos.setLatScale(bestMapTile->getLatScale());
          newDisplayArea.setRefPos(refPos);
          newDisplayArea.setZoomLevel(bestMapTile->getParentMapContainer()->getZoomLevelMap());
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
          visPosRW=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
          this->visPos=*visPosRW;          
          core->getDefaultGraphicEngine()->unlockPos();
          lockMapPos(__FILE__, __LINE__);
          mapPos=newMapPos;
          unlockMapPos();

          // Set the new display area
          MapArea oldDisplayArea=displayArea;
          lockDisplayArea(__FILE__, __LINE__);
          //DEBUG("newDisplayArea.getZoomLevel()=%d",newDisplayArea.getZoomLevel());
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
          lockCenterMapTiles(__FILE__,__LINE__);
          centerMapTiles.clear();
          unlockCenterMapTiles();
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
              core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
              t->activateVisPos();
              core->getDefaultGraphicEngine()->unlockDrawing();

              // If the tile is hidden: make it visible
              if (t->getIsHidden()) {
                t->setIsHidden(false,__FILE__, __LINE__);
              }
            }
          }
          //PROFILE_ADD("tile list update");

          // If no tile has been found for whatever reason, reset the map
          //DEBUG("tiles.size()=%d zoomedScreenWidth=%d",tiles.size(),zoomedScreenWidth);
          if ((!abortUpdate)&&(tiles.size()==0)) {
            DEBUG("resetting map",NULL);
            core->getConfigStore()->setStringValue("Map/LastPosition","folder","*unknown*",__FILE__, __LINE__);
            initMap();
          }

          // Request cache and map tile overlay graphic update
          mapChanged=true;
        }
      }

    } else {

      // Reset the visual position to the previous one
      DEBUG("no map tile found",NULL);
      lockMapPos(__FILE__, __LINE__);
      mapPos.setMapTile(NULL);
      unlockMapPos();
      visPosRW=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
      *visPosRW=this->visPos;
      core->getDefaultGraphicEngine()->unlockPos();
      //PROFILE_ADD("no tile found");

    }
    core->getMapSource()->unlockAccess();

  }

  // Do we need to update the navigation graphic?
  if ((mapChanged)||(core->getNavigationEngine()->mapGraphicUpdateIsRequired(__FILE__,__LINE__))) {
    core->getNavigationEngine()->updateMapGraphic();
  }

  // Was the map changed?
  if (mapChanged) {

    // Update any widgets
    lockCenterMapTiles(__FILE__,__LINE__);
    core->onMapChange(mapPos,&centerMapTiles);
    unlockCenterMapTiles();

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
  GraphicPosition visPos=*(core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__));
  core->getDefaultGraphicEngine()->unlockPos();

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
  int len=core->getDefaultScreen()->getHeight();
  if (core->getDefaultScreen()->getWidth()>len)
    len=core->getDefaultScreen()->getWidth();
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

// Sets the zoom level
void MapEngine::setZoomLevel(Int zoomLevel) {
  DEBUG("setZoomLevel(%d) called",zoomLevel);
  setZoomLevelLock(true,false);
  lockDisplayArea(__FILE__,__LINE__);
  displayArea.setZoomLevel(zoomLevel);
  unlockDisplayArea();
  lockMapPos(__FILE__,__LINE__);
  MapPosition mapPos=this->mapPos;
  unlockMapPos();
  core->getMapSource()->lockAccess(__FILE__,__LINE__);
  MapTile *bestMapTile=core->getMapSource()->findMapTileByGeographicCoordinate(mapPos,zoomLevel,zoomLevelLock);
  core->getMapSource()->unlockAccess();
  if (bestMapTile) {
    lockMapPos(__FILE__,__LINE__);
    this->mapPos.setLngScale(bestMapTile->getLngScale());
    this->mapPos.setLatScale(bestMapTile->getLatScale());
    unlockMapPos();
  }
  setForceMapRecreation();
}

// Convert the distance in meters to pixels for the given map state
bool MapEngine::calculateDistanceInScreenPixels(MapPosition src, MapPosition dst, double &distance) {  
  GraphicPosition *visPos=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
  double zoom=visPos->getZoom();
  core->getDefaultGraphicEngine()->unlockPos();
  MapPosition pos=*(core->getMapEngine()->lockMapPos(__FILE__, __LINE__));
  core->getMapEngine()->unlockMapPos();
  MapCalibrator *calibrator=NULL;
  bool result=false;
  if ((pos.getLngScale()>0)&&(pos.getMapTile())&&(pos.getMapTile()->getParentMapContainer())&&((calibrator=pos.getMapTile()->getParentMapContainer()->getMapCalibrator())!=NULL)) {    
    calibrator->setPictureCoordinates(src);
    calibrator->setPictureCoordinates(dst);
    Int xDist=dst.getX()-src.getX();
    Int yDist=dst.getY()-src.getY();
    distance=sqrt(xDist*xDist+yDist*yDist)*zoom;
    //DEBUG("src=(%f,%f) dst=(%f,%f) xDist=%d yDist=%d zoom=%f distance=%f",src.getLat(),src.getLng(),dst.getLat(),dst.getLng(),xDist,yDist,zoom,distance);
    result=true;
  }
  return result;
}

// Convert the distance in pixels to max possible meters for the given map state
bool MapEngine::calculateMaxDistanceInMeters(Int distanceInPixels, double &distanceInMeters) {  
  GraphicPosition *visPos=core->getDefaultGraphicEngine()->lockPos(__FILE__, __LINE__);
  double zoom=visPos->getZoom();
  core->getDefaultGraphicEngine()->unlockPos();
  MapPosition pos=*(core->getMapEngine()->lockMapPos(__FILE__, __LINE__));
  core->getMapEngine()->unlockMapPos();
  MapCalibrator *calibrator=NULL;
  bool result=false;
  if ((pos.getLngScale()>0)&&(pos.getMapTile())&&(pos.getMapTile()->getParentMapContainer())&&((calibrator=pos.getMapTile()->getParentMapContainer()->getMapCalibrator())!=NULL)) {    
    //DEBUG("distanceInPixels=%d zoom=%f",distanceInPixels,zoom);
    double t=((double)distanceInPixels)/zoom;
    distanceInPixels=round(t);
    //DEBUG("distanceInPixels=%d",distanceInPixels);
    MapPosition src;
    src.setX(0);
    src.setY(0);
    MapPosition dst;
    dst.setX(distanceInPixels);
    dst.setY(distanceInPixels);
    calibrator->setGeographicCoordinates(src);
    calibrator->setGeographicCoordinates(dst);
    distanceInMeters=src.computeDistance(dst);
    //DEBUG("distanceInMeters=%f",distanceInMeters);
    result=true;
  }
  return result;
}

}
