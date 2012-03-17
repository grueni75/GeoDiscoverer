//============================================================================
// Name        : NavigationPathVisualization.cpp
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
NavigationPathVisualization::NavigationPathVisualization() {
  prevLinePoint=NavigationPath::getPathInterruptedPos();
  prevArrowPoint=NavigationPath::getPathInterruptedPos();
}

// Destructor
NavigationPathVisualization::~NavigationPathVisualization() {

  // Remove all visualizations from the tiles
  for(NavigationPathTileInfoMap::iterator i=tileInfoMap.begin();i!=tileInfoMap.end();i++) {
    MapTile *mapTile=i->first;
    NavigationPathTileInfo *tileInfo=i->second;
    GraphicObject *tileVisualization=mapTile->getVisualization();
    tileVisualization->lockAccess();
    if (tileInfo->getGraphicLine())
      tileVisualization->removePrimitive(tileInfo->getGraphicLineKey(),true);
    if (tileInfo->getGraphicRectangeList())
      tileVisualization->removePrimitive(tileInfo->getGraphicRectangleListKey(),true);
    tileVisualization->unlockAccess();
    delete tileInfo;
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

// Indicates that textures and buffers have been invalidated
void NavigationPathVisualization::graphicInvalidated() {
  for(NavigationPathTileInfoMap::iterator i=tileInfoMap.begin();i!=tileInfoMap.end();i++) {
    GraphicLine *line=i->second->getGraphicLine();
    if (line) line->invalidate();
    GraphicRectangleList *rectangleList=i->second->getGraphicRectangeList();
    if (rectangleList) rectangleList->invalidate();
  }
}

// Recreate the graphic objects to reduce the number of graphic point buffers
void NavigationPathVisualization::optimizeGraphic() {
  for(NavigationPathTileInfoMap::iterator i=tileInfoMap.begin();i!=tileInfoMap.end();i++) {
    i->first->getVisualization()->lockAccess();
    GraphicLine *line=i->second->getGraphicLine();
    if (line) line->optimize();
    GraphicRectangleList *rectangleList=i->second->getGraphicRectangeList();
    if (rectangleList) rectangleList->optimize();
    i->first->getVisualization()->unlockAccess();
  }
}

// Compute the distance in pixels between two points
double NavigationPathVisualization::computePixelDistance(MapPosition a, MapPosition b) {
  double lngDiff=b.getLng()-a.getLng();
  lngDiff*=lngScale;
  double latDiff=b.getLat()-a.getLat();
  latDiff*=latScale;
  return sqrt(lngDiff*lngDiff+latDiff*latDiff);
}

}
