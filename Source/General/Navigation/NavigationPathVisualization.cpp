//============================================================================
// Name        : NavigationPathVisualization.cpp
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

// Constructor
NavigationPathVisualization::NavigationPathVisualization() {
  prevLinePoint=NavigationPath::getPathInterruptedPos();
  prevArrowPoint=NavigationPath::getPathInterruptedPos();
  lngScale=0;
  latScale=0;
  zoomLevel=0;
}

// Destructor
NavigationPathVisualization::~NavigationPathVisualization() {

  // Remove all visualizations from the tiles
  while (tileInfoMap.size()>0) {
    NavigationPathTileInfoMap::iterator i = tileInfoMap.begin();
    MapTile *mapTile=i->first;
    NavigationPathTileInfo *tileInfo=i->second;
    removeTileInfo(mapTile);
    delete tileInfo;
  }
}

// Removes the tile from the visualization
void NavigationPathVisualization::removeTileInfo(MapTile *tile) {
  NavigationPathTileInfoMap::iterator i;
  i=tileInfoMap.find(tile);
  if (i!=tileInfoMap.end()) {
    NavigationPathTileInfo *tileInfo=i->second;
    GraphicObject *tileVisualization=tile->getVisualization();
    core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
    if (tileInfo->getPathLine())
      tileVisualization->removePrimitive(tileInfo->getPathLineKey(),true);
    if (tileInfo->getPathArrowList())
      tileVisualization->removePrimitive(tileInfo->getPathArrowListKey(),true);
    if (tileInfo->getPathStartFlag())
      tileVisualization->removePrimitive(tileInfo->getPathStartFlagKey(),true);
    if (tileInfo->getPathEndFlag())
      tileVisualization->removePrimitive(tileInfo->getPathEndFlagKey(),true);
    core->getDefaultGraphicEngine()->unlockDrawing();
    tileInfoMap.erase(i);
  }
}

// Returns the tile info for the given tile
NavigationPathTileInfo *NavigationPathVisualization::findTileInfo(MapTile *tile) {

  // First check if it is already in the map
  NavigationPathTileInfoMap::iterator i;
  i=tileInfoMap.find(tile);
  if (i==tileInfoMap.end()) {

    // No, create a new one
    NavigationPathTileInfo *info=new NavigationPathTileInfo();
    tileInfoMap.insert(NavigationPathTileInfoPair(tile,info));
    return info;

  } else {

    // Yes, return the existing one
    return i->second;

  }


}

// Indicates that textures and buffers have been cleared
void NavigationPathVisualization::destroyGraphic() {
  for(NavigationPathTileInfoMap::iterator i=tileInfoMap.begin();i!=tileInfoMap.end();i++) {
    NavigationPathTileInfo *tileInfo=i->second;
    GraphicLine *line=tileInfo->getPathLine();
    if (line) line->invalidate();
    GraphicRectangleList *rectangleList=tileInfo->getPathArrowList();
    if (rectangleList) {
      rectangleList->invalidate();
    }
    GraphicRectangle *rectangle=tileInfo->getPathStartFlag();
    if (rectangle) {
      rectangle->invalidate();
    }
    rectangle=tileInfo->getPathEndFlag();
    if (rectangle) {
      rectangle->invalidate();
    }
  }
}

// Indicates that textures and buffers shall be created
void NavigationPathVisualization::createGraphic() {
  for(NavigationPathTileInfoMap::iterator i=tileInfoMap.begin();i!=tileInfoMap.end();i++) {
    NavigationPathTileInfo *tileInfo=i->second;
    GraphicRectangleList *rectangleList=tileInfo->getPathArrowList();
    if (rectangleList) {
      rectangleList->setTexture(core->getDefaultGraphicEngine()->getPathDirectionIcon()->getTexture());
    }
    GraphicRectangle *rectangle=tileInfo->getPathStartFlag();
    if (rectangle) {
      rectangle->setTexture(core->getDefaultGraphicEngine()->getPathStartFlagIcon()->getTexture());
    }
    rectangle=tileInfo->getPathEndFlag();
    if (rectangle) {
      rectangle->setTexture(core->getDefaultGraphicEngine()->getPathEndFlagIcon()->getTexture());
    }
  }
}

// Recreate the graphic objects to reduce the number of graphic point buffers
void NavigationPathVisualization::optimizeGraphic() {
  for(NavigationPathTileInfoMap::iterator i=tileInfoMap.begin();i!=tileInfoMap.end();i++) {
    GraphicLine *line=i->second->getPathLine();
    core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
    if (line) line->optimize();
    core->getDefaultGraphicEngine()->unlockDrawing();
    GraphicRectangleList *rectangleList=i->second->getPathArrowList();
    core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
    if (rectangleList) rectangleList->optimize();
    core->getDefaultGraphicEngine()->unlockDrawing();
    GraphicRectangle *rectangle=i->second->getPathStartFlag();
    core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
    if (rectangle) rectangle->optimize();
    core->getDefaultGraphicEngine()->unlockDrawing();
    rectangle=i->second->getPathEndFlag();
    core->getDefaultGraphicEngine()->lockDrawing(__FILE__, __LINE__);
    if (rectangle) rectangle->optimize();
    core->getDefaultGraphicEngine()->unlockDrawing();
  }
}

// Compute the distance in pixels between two points
MapCalibrator *NavigationPathVisualization::findCalibrator(MapPosition pos, bool &deleteCalibrator) {
  core->getMapSource()->lockAccess(__FILE__,__LINE__);
  MapCalibrator *c=core->getMapSource()->findMapCalibrator(zoomLevel,pos,deleteCalibrator);
  core->getMapSource()->unlockAccess();
  return c;
}

// Adds a new point to the visualization
void NavigationPathVisualization::addPoint(MapPosition pos) {
  if (pos.getHasBearing()) {
    prevArrowPoint=pos;
  }
  points.push_back(pos);
  prevLinePoint=pos;
}

}
